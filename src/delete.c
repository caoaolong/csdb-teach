#define _POSIX_C_SOURCE 200809L /* fseeko 需要 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "delete.h"
#include "engine.h"
#include "index.h"
#include "record.h"

int delete_db_row(const char *db_path, const char *id_str)
{
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int64_t qid = 0;
    if (!parse_id(id_str, &qid) || qid <= 0)
    {
        fprintf(stderr, "ID 非法: %s\n", id_str);
        return 1;
    }

    /* 二分定位 */
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
        fprintf(stderr, "未找到 ID 为 %s 的行（或已被删除）\n", id_str);
        return 1;
    }
    if (rc < 0)
    {
        return 1; /* 索引损坏，错误已打印 */
    }

    FILE *fp = fopen(db_path, "r+b");
    if (fp == NULL)
    {
        perror("打开文件失败");
        return 1;
    }
    if (fseeko(fp, target_off, SEEK_SET) != 0)
    {
        perror("数据文件 seek 失败");
        fclose(fp);
        return 1;
    }
    student_t r;
    if (fread(&r, sizeof(r), 1, fp) != 1)
    {
        perror("读取记录失败");
        fclose(fp);
        return 1;
    }
    /* 回验：offset 处 id 必须等于 qid，否则索引与数据不一致 */
    if (r.id != qid)
    {
        fprintf(stderr, "索引与数据不一致，请重建索引\n");
        fclose(fp);
        return 1;
    }
    if (fseeko(fp, target_off, SEEK_SET) != 0)
    {
        perror("数据文件 seek 失败");
        fclose(fp);
        return 1;
    }
    /* 删除行标志标记：只改 8 字节 id */
    int64_t tomb = TOMBSTONE_ID;
    if (fwrite(&tomb, sizeof(tomb), 1, fp) != 1)
    {
        perror("写入删除行标志失败");
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

    if (build_index(db_path) != 0)
    {
        fprintf(stderr, "数据已标记删除，但索引重建失败，请手动执行 index\n");
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "已删除 ID=%s (删除行标志标记，索引已重建)\n", id_str);
    fprintf(stderr, "删除用时: %.3f ms (二分定位 + 删除行标志写 + 索引重建)\n", elapsed_ms);
    return 0;
}
