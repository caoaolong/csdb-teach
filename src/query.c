#define _POSIX_C_SOURCE 200809L /* clock_gettime 需要 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "engine.h"
#include "index.h"
#include "query.h"
#include "record.h"

int query_db(const char *db_path, const char *id_str)
{
    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int64_t qid = 0;
    if (!parse_id(id_str, &qid))
    {
        fprintf(stderr, "未找到 ID 为 %s 的行\n", id_str);
        return 1;
    }

    int64_t target_off = -1;
    int rc = idx_lookup(db_path, qid, &target_off);
    if (rc == -1)
    {
        return query_scan(db_path, id_str, t0);
    }
    if (rc < 0)
    {
        return 1; /* 索引损坏，错误已打印 */
    }
    if (rc == 0)
    {
        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                            (t1.tv_nsec - t0.tv_nsec) / 1e6;
        fprintf(stderr, "查询用时: %.3f ms (索引命中失败)\n", elapsed_ms);
        fprintf(stderr, "未找到 ID 为 %s 的行\n", id_str);
        return 1;
    }

    FILE *fp = fopen(db_path, "rb");
    if (fp == NULL)
    {
        perror("打开文件失败");
        return 1;
    }
    db_header_t hdr;
    if (db_read_header(fp, &hdr) != 0 || hdr.row_size != sizeof(student_t))
    {
        fprintf(stderr, "不是 CSDB 数据文件或版本不支持: %s\n", db_path);
        fclose(fp);
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
    fclose(fp);

    /* 回验：offset 处 id 必须等于 qid，否则索引与数据不一致 */
    if (r.id != qid)
    {
        fprintf(stderr, "索引与数据不一致，回退全表扫描，请重建索引\n");
        return query_scan(db_path, id_str, t0);
    }
    db_print_row(&r);

    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "查询用时: %.3f ms (索引命中)\n", elapsed_ms);
    return 0;
}

/* 全表扫描：无索引时的回退路径 */
int query_scan(const char *db_path, const char *id_str, struct timespec t0)
{
    int64_t qid = 0;
    if (!parse_id(id_str, &qid))
    {
        fprintf(stderr, "未找到 ID 为 %s 的行\n", id_str);
        return 1;
    }

    FILE *fp = fopen(db_path, "rb");
    if (fp == NULL)
    {
        perror("打开文件失败");
        return 1;
    }
    db_header_t hdr;
    if (db_read_header(fp, &hdr) != 0 || hdr.row_size != sizeof(student_t))
    {
        fprintf(stderr, "不是 CSDB 数据文件或版本不支持: %s\n", db_path);
        fclose(fp);
        return 1;
    }

    student_t r;
    int found = 0;
    for (uint64_t i = 0; i < hdr.row_count; i++)
    {
        if (fread(&r, sizeof(r), 1, fp) != 1)
        {
            perror("读取记录失败");
            fclose(fp);
            return 1;
        }
        if (r.id == qid)
        {
            db_print_row(&r);
            found = 1;
            break;
        }
    }
    fclose(fp);

    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "查询用时: %.3f ms (定长扫描)\n", elapsed_ms);

    if (!found)
    {
        fprintf(stderr, "未找到 ID 为 %s 的行\n", id_str);
        return 1;
    }
    return 0;
}
