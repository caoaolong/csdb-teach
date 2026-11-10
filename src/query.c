#define _POSIX_C_SOURCE 200809L /* clock_gettime 需要 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "engine.h"
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
