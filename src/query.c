#define _POSIX_C_SOURCE 200809L /* clock_gettime 需要 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"
#include "index.h"
#include "query.h"

/* 全表扫描：与原 main.c 逻辑一致，作为无索引时的回退路径 */
static int query_scan(const char *csv_path, const char *id_str,
                      struct timespec t0, int timing_started)
{
    FILE *fp = fopen(csv_path, "rb");
    if (fp == NULL)
    {
        perror("打开文件失败");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    char field[1024];
    int found = 0;
    int lineno = 0;

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        lineno++;
        const char *p = skip_bom(line, (size_t)nread, lineno == 1);
        first_field(p, field, sizeof(field));
        if (strcmp(field, id_str) == 0)
        {
            fputs(line, stdout);
            found = 1;
            break;
        }
    }

    free(line);
    fclose(fp);

    if (timing_started)
    {
        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                            (t1.tv_nsec - t0.tv_nsec) / 1e6;
        fprintf(stderr, "查询用时: %.3f ms (全表扫描)\n", elapsed_ms);
    }

    if (!found)
    {
        fprintf(stderr, "未找到 ID 为 %s 的行\n", id_str);
        return 1;
    }
    return 0;
}

int query_csv(const char *csv_path, const char *id_str)
{
    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* 非纯数字 ID 无法查数字索引，直接走扫描（保持原 strcmp 语义） */
    int64_t qid = 0;
    if (!parse_id(id_str, &qid))
    {
        return query_scan(csv_path, id_str, t0, 1);
    }

    /* 索引查找：具体的文件格式与二分逻辑都在 index.c 内 */
    int64_t target_off = -1;
    int rc = idx_lookup(csv_path, qid, &target_off);
    if (rc == -1)
    {
        /* 无索引文件：回退全表扫描 */
        return query_scan(csv_path, id_str, t0, 1);
    }
    if (rc < 0)
    {
        return 1; /* 索引损坏或读写失败，错误已由 idx_lookup 打印 */
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

    FILE *fp = fopen(csv_path, "rb");
    if (fp == NULL)
    {
        perror("打开文件失败");
        return 1;
    }
    if (fseeko(fp, (long long)target_off, SEEK_SET) != 0)
    {
        perror("数据文件 seek 失败");
        fclose(fp);
        return 1;
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t nread = getline(&line, &len, fp);
    if (nread == -1)
    {
        perror("读取数据行失败");
        free(line);
        fclose(fp);
        return 1;
    }
    /* 读时回验：offset 处第一字段必须等于 qid，否则索引已过期，不可直接返回 */
    {
        const char *p = skip_bom(line, (size_t)nread, target_off == 0);
        char field[1024];
        first_field(p, field, sizeof(field));
        int64_t got = 0;
        if (!parse_id(field, &got) || got != qid)
        {
            fprintf(stderr, "索引与数据不一致(可能已过期)，回退全表扫描，请重建索引\n");
            free(line);
            fclose(fp);
            return query_scan(csv_path, id_str, t0, 1);
        }
    }
    fputs(line, stdout);
    free(line);
    fclose(fp);

    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "查询用时: %.3f ms (索引命中)\n", elapsed_ms);
    return 0;
}
