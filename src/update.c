#define _POSIX_C_SOURCE 200809L /* fseeko 需要 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"
#include "index.h"
#include "record.h"
#include "update.h"

typedef enum
{
    COL_TEXT,
    COL_INT32,
} col_type_t;

typedef struct
{
    const char *name;
    size_t off;  /* 列在 student_t 中的字节偏移 */
    size_t size; /* 列宽（文本列含 NUL） */
    col_type_t type;
} col_info_t;

static const col_info_t COLS[] = {
    {"id", offsetof(student_t, id), sizeof(((student_t *)0)->id), COL_INT32}, /* 仅定位用，禁止改 */
    {"student_no", offsetof(student_t, student_no), sizeof(((student_t *)0)->student_no), COL_TEXT},
    {"name", offsetof(student_t, name), sizeof(((student_t *)0)->name), COL_TEXT},
    {"gender", offsetof(student_t, gender), sizeof(((student_t *)0)->gender), COL_TEXT},
    {"age", offsetof(student_t, age), sizeof(((student_t *)0)->age), COL_INT32},
    {"major", offsetof(student_t, major), sizeof(((student_t *)0)->major), COL_TEXT},
    {"grade", offsetof(student_t, grade), sizeof(((student_t *)0)->grade), COL_TEXT},
    {"course", offsetof(student_t, course), sizeof(((student_t *)0)->course), COL_TEXT},
    {"score", offsetof(student_t, score), sizeof(((student_t *)0)->score), COL_INT32},
    {"email", offsetof(student_t, email), sizeof(((student_t *)0)->email), COL_TEXT},
    {"phone", offsetof(student_t, phone), sizeof(((student_t *)0)->phone), COL_TEXT},
};
#define NCOL (sizeof(COLS) / sizeof(COLS[0]))

static const col_info_t *find_col(const char *name)
{
    for (size_t i = 0; i < NCOL; i++)
    {
        if (strcmp(name, COLS[i].name) == 0)
        {
            return &COLS[i];
        }
    }
    return NULL;
}

static void usage_cols(void)
{
    fprintf(stderr, "可用列: id(禁改), student_no, name, gender, age, major, grade, course, score, email, phone\n");
}

int update_db_field(const char *db_path, const char *id_str,
                    const char *column, const char *value)
{
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int64_t qid = 0;
    if (!parse_id(id_str, &qid))
    {
        fprintf(stderr, "ID 非法: %s\n", id_str);
        return 1;
    }
    const col_info_t *col = find_col(column);
    if (col == NULL)
    {
        fprintf(stderr, "未知列: %s\n", column);
        usage_cols();
        return 1;
    }
    if (strcmp(column, "id") == 0)
    {
        /* id 是索引排序键，改它等于删了重插，本课不做 */
        fprintf(stderr, "id 列禁止修改（会破坏索引顺序）\n");
        return 1;
    }

    /* 按类型准备“恰好一列宽”的写入缓冲（文本零填充截断，整数严格解析） */
    unsigned char buf[64]; /* 文本列最大 64 字节 */
    if (col->size > sizeof(buf))
    {
        fprintf(stderr, "内部错误：列宽超出缓冲\n");
        return 1;
    }
    if (col->type == COL_INT32)
    {
        char *end = NULL;
        long v = strtol(value, &end, 10);
        if (end == value || *end != '\0')
        {
            fprintf(stderr, "列 %s 需要整数，当前: %s\n", column, value);
            return 1;
        }
        if (v > INT32_MAX || v < INT32_MIN)
        {
            fprintf(stderr, "列 %s 数值越界: %s\n", column, value);
            return 1;
        }
        int32_t i32 = (int32_t)v; /* age/score 列宽即 4 */
        memset(buf, 0, col->size);
        memcpy(buf, &i32, col->size);
    }
    else
    {
        memset(buf, 0, col->size);
        strncpy((char *)buf, value, col->size - 1);
    }

    /* 二分定位：无索引或已过期则先重建，保证 offset 新鲜 */
    int64_t target_off = -1;
    int rc = idx_lookup(db_path, qid, &target_off);
    if (rc == -1)
    {
        if (build_index(db_path) != 0)
        {
            return 1;
        }
        rc = idx_lookup(db_path, qid, &target_off);
    }
    if (rc == 0)
    {
        fprintf(stderr, "未找到 ID 为 %s 的行\n", id_str);
        return 1;
    }
    if (rc < 0)
    {
        return 1; /* 索引损坏，错误已打印 */
    }

    /* 定长原地写：行长度不变，后面所有行的 offset 都不受影响 */
    FILE *fp = fopen(db_path, "r+b");
    if (fp == NULL)
    {
        perror("打开文件失败");
        return 1;
    }
    if (fseeko(fp, target_off + (long long)col->off, SEEK_SET) != 0)
    {
        perror("数据文件 seek 失败");
        fclose(fp);
        return 1;
    }
    if (fwrite(buf, col->size, 1, fp) != 1)
    {
        perror("写入记录失败");
        fclose(fp);
        return 1;
    }
    fflush(fp);
    if (file_fsync(fp) != 0)
    {
        perror("数据 fsync 失败");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    /* 索引的更新：条目一个没动（offset 全有效），只刷新头的 size/mtime；
     * 否则下次 lookup 会因 mtime 对不上判过期 */
    if (idx_refresh(db_path) != 0)
    {
        fprintf(stderr, "数据已更新，但索引头刷新失败，请手动执行 index\n");
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "已更新 ID=%s 的 %s 为 %s (原地写，索引条目不变)\n",
            id_str, column, value);
    fprintf(stderr, "更新用时: %.3f ms (二分定位 + 原地写)\n", elapsed_ms);
    return 0;
}
