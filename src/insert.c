#define _POSIX_C_SOURCE 200809L /* fseeko / clock_gettime 需要 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"
#include "index.h"
#include "insert.h"
#include "record.h"

enum
{
    C_ID = 0,
    C_STUDENT_NO = 1,
    C_NAME = 2,
    C_GENDER = 3,
    C_AGE = 4,
    C_MAJOR = 5,
    C_GRADE = 6,
    C_COURSE = 7,
    C_SCORE = 8,
    C_EMAIL = 9,
    C_PHONE = 10,
    C_NCOL = 11
};

static void set_text(char *dst, size_t cap, const char *v)
{
    strncpy(dst, v, cap - 1);
    dst[cap - 1] = '\0';
}

static int strict_i32(const char *v, const char *col, int32_t *out)
{
    char *end = NULL;
    long x = strtol(v, &end, 10);
    if (end == v || *end != '\0')
    {
        fprintf(stderr, "列 %s 需要整数，当前: %s\n", col, v);
        return 0;
    }
    if (x > INT32_MAX || x < INT32_MIN)
    {
        fprintf(stderr, "列 %s 数值越界: %s\n", col, v);
        return 0;
    }
    *out = (int32_t)x;
    return 1;
}

int insert_db_row(const char *db_path, const char *csv_line)
{
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* 1. 解析 CSV 行（文本列零填充截断，整数严格校验） */
    size_t len = strlen(csv_line);
    char v[256];
    student_t nr;
    memset(&nr, 0, sizeof(nr));

    int64_t qid = 0;
    if (!csv_value(csv_line, len, C_ID, v, sizeof(v)) ||
        !parse_id(v, &qid) || qid <= 0)
    {
        fprintf(stderr, "ID 非法: %s\n", v);
        return 1;
    }
    nr.id = qid;
    if (!csv_value(csv_line, len, C_STUDENT_NO, v, sizeof(v)))
        goto missing;
    set_text(nr.student_no, sizeof(nr.student_no), v);
    if (!csv_value(csv_line, len, C_NAME, v, sizeof(v)))
        goto missing;
    set_text(nr.name, sizeof(nr.name), v);
    if (!csv_value(csv_line, len, C_GENDER, v, sizeof(v)))
        goto missing;
    set_text(nr.gender, sizeof(nr.gender), v);
    if (!csv_value(csv_line, len, C_AGE, v, sizeof(v)))
        goto missing;
    if (!strict_i32(v, "age", &nr.age))
        return 1;
    if (!csv_value(csv_line, len, C_MAJOR, v, sizeof(v)))
        goto missing;
    set_text(nr.major, sizeof(nr.major), v);
    if (!csv_value(csv_line, len, C_GRADE, v, sizeof(v)))
        goto missing;
    set_text(nr.grade, sizeof(nr.grade), v);
    if (!csv_value(csv_line, len, C_COURSE, v, sizeof(v)))
        goto missing;
    set_text(nr.course, sizeof(nr.course), v);
    if (!csv_value(csv_line, len, C_SCORE, v, sizeof(v)))
        goto missing;
    if (!strict_i32(v, "score", &nr.score))
        return 1;
    if (!csv_value(csv_line, len, C_EMAIL, v, sizeof(v)))
        goto missing;
    set_text(nr.email, sizeof(nr.email), v);
    if (!csv_value(csv_line, len, C_PHONE, v, sizeof(v)))
        goto missing;
    set_text(nr.phone, sizeof(nr.phone), v);

    /* 2. 判重：索引里已存在则拒绝（无索引/过期先重建） */
    {
        int64_t dummy = -1;
        int rc = idx_lookup(db_path, qid, &dummy);
        if (rc == -1)
        {
            if (build_index(db_path) != 0)
            {
                return 1;
            }
            rc = idx_lookup(db_path, qid, &dummy);
        }
        if (rc == 1)
        {
            fprintf(stderr, "ID 已存在: %lld\n", (long long)qid);
            return 1;
        }
        if (rc < 0)
        {
            return 1; /* 索引损坏，错误已打印 */
        }
    }

    /* 3. 找删除行标志槽位复用，否则追加到末尾 */
    FILE *fp = fopen(db_path, "r+b");
    if (fp == NULL)
    {
        perror("打开文件失败");
        return 1;
    }
    db_header_t hdr;
    if (fseeko(fp, 0, SEEK_SET) != 0 || db_read_header(fp, &hdr) != 0 ||
        hdr.row_size != sizeof(student_t))
    {
        fprintf(stderr, "不是 CSDB 数据文件或版本不支持: %s\n", db_path);
        fclose(fp);
        return 1;
    }
    if (fseeko(fp, 0, SEEK_END) != 0)
    {
        perror("数据文件 seek 失败");
        fclose(fp);
        return 1;
    }
    long long fsize = ftello(fp);
    long long expect = (long long)sizeof(db_header_t) +
                       (long long)hdr.row_count * (long long)sizeof(student_t);
    if (fsize != expect)
    {
        fprintf(stderr, "数据文件大小与头不一致（可能损坏），请检查: %s\n", db_path);
        fclose(fp);
        return 1;
    }

    int64_t slot_off = -1;
    student_t r;
    for (uint64_t i = 0; i < hdr.row_count; i++)
    {
        long long off = (long long)sizeof(db_header_t) +
                        (long long)i * (long long)sizeof(student_t);
        if (fseeko(fp, off, SEEK_SET) != 0 || fread(&r, sizeof(r), 1, fp) != 1)
        {
            perror("读取记录失败");
            fclose(fp);
            return 1;
        }
        if (r.id <= 0)
        {
            slot_off = (int64_t)off; /* 第一个删除行标志槽位 */
            break;
        }
    }

    uint64_t old_count = hdr.row_count;
    int reused = (slot_off >= 0);
    int64_t write_off = reused ? slot_off : expect;
    if (fseeko(fp, write_off, SEEK_SET) != 0 ||
        fwrite(&nr, sizeof(nr), 1, fp) != 1)
    {
        perror("写入记录失败");
        fclose(fp);
        return 1;
    }
    if (!reused)
    {
        /* 先写行、再回填头：崩溃最多留一行 row_count 之外的孤儿行，无害 */
        hdr.row_count = old_count + 1;
        if (fseeko(fp, 0, SEEK_SET) != 0 ||
            fwrite(&hdr, sizeof(hdr), 1, fp) != 1)
        {
            perror("回填文件头失败");
            fclose(fp);
            return 1;
        }
    }
    fflush(fp);
    if (file_fsync(fp) != 0)
    {
        perror("数据 fsync 失败");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    /* 4. id/offset 变化，只刷头不够，必须重建索引 */
    if (build_index(db_path) != 0)
    {
        fprintf(stderr, "数据已写入，但索引重建失败，请手动执行 index\n");
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    if (reused)
    {
        fprintf(stderr, "已插入 ID=%lld (复用删除行标志槽位 offset=%lld，行数不变 %llu)\n",
                (long long)qid, (long long)write_off,
                (unsigned long long)old_count);
    }
    else
    {
        fprintf(stderr, "已插入 ID=%lld (追加到末尾，行数 %llu -> %llu)\n",
                (long long)qid,
                (unsigned long long)old_count,
                (unsigned long long)(old_count + 1));
    }
    fprintf(stderr, "插入用时: %.3f ms (判重 + 写行 + 索引重建)\n", elapsed_ms);
    return 0;

missing:
    fprintf(stderr, "CSV 行缺少列（需要 %d 列：id,student_no,name,gender,age,major,grade,course,score,email,phone）\n", C_NCOL);
    return 1;
}
