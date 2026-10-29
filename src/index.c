#define _POSIX_C_SOURCE 200809L /* getline / ftello 需要 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"
#include "index.h"

static int parse_id(const char *field, int64_t *out)
{
    char *end = NULL;
    errno = 0;
    if (field[0] == '\0')
    {
        return 0;
    }
    long long v = strtoll(field, &end, 10);
    if (errno != 0 || end == field || *end != '\0')
    {
        return 0;
    }
    *out = (int64_t)v;
    return 1;
}

static int cmp_entry(const void *a, const void *b)
{
    const idx_entry_t *x = (const idx_entry_t *)a;
    const idx_entry_t *y = (const idx_entry_t *)b;
    if (x->id < y->id)
    {
        return -1;
    }
    if (x->id > y->id)
    {
        return 1;
    }
    return 0;
}

void derive_idx_path(const char *csv_path, char *out, size_t cap)
{
    const char *slash = strrchr(csv_path, '/');
    const char *backslash = strrchr(csv_path, '\\');
    const char *dir_end = slash;
    if (backslash != NULL && (dir_end == NULL || backslash > dir_end))
    {
        dir_end = backslash;
    }
    const char *base = (dir_end != NULL) ? dir_end + 1 : csv_path;
    size_t dir_len = (dir_end != NULL) ? (size_t)(dir_end - csv_path + 1) : 0;

    const char *dot = strrchr(base, '.');
    size_t base_len = (dot != NULL) ? (size_t)(dot - base) : strlen(base);

    /* dir + base(去后缀) + ".idx" */
    size_t need = dir_len + base_len + 4 + 1;
    if (need > cap)
    {
        /* 截断保底：只保证 NUL 结尾 */
        if (cap > 0)
        {
            out[0] = '\0';
        }
        return;
    }
    memcpy(out, csv_path, dir_len);
    memcpy(out + dir_len, base, base_len);
    memcpy(out + dir_len + base_len, ".idx", 5); /* 含 NUL */
}

int build_index(const char *csv_path)
{
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    FILE *fp = fopen(csv_path, "rb");
    if (fp == NULL)
    {
        perror("打开文件失败");
        return 1;
    }

    size_t cap = 1 << 20; /* 预分配约 100 万行，16MB */
    size_t cnt = 0;
    idx_entry_t *arr = (idx_entry_t *)malloc(cap * sizeof(idx_entry_t));
    if (arr == NULL)
    {
        fprintf(stderr, "内存分配失败\n");
        fclose(fp);
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    char field[1024];
    long lineno = 0;

    while (1)
    {
        long long off = ftello(fp); /* 本行行首偏移，必须在 getline 之前取 */
        if (off < 0)
        {
            perror("ftello 失败");
            free(arr);
            free(line);
            fclose(fp);
            return 1;
        }
        nread = getline(&line, &len, fp);
        if (nread == -1)
        {
            break;
        }
        lineno++;
        const char *p = line;
        if (lineno == 1 && nread >= 3 &&
            (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB &&
            (unsigned char)p[2] == 0xBF)
        {
            p += 3; /* 跳过 UTF-8 BOM（csdb-gd 默认 utf8-bom 编码） */
        }
        first_field(p, field, sizeof(field));
        int64_t id;
        if (!parse_id(field, &id))
        {
            continue; /* 跳过表头 / 非数字 ID 行 */
        }
        if (cnt == cap)
        {
            cap *= 2;
            idx_entry_t *narr = (idx_entry_t *)realloc(arr, cap * sizeof(idx_entry_t));
            if (narr == NULL)
            {
                fprintf(stderr, "内存分配失败\n");
                free(arr);
                free(line);
                fclose(fp);
                return 1;
            }
            arr = narr;
        }
        arr[cnt].id = id;
        arr[cnt].offset = (int64_t)off;
        cnt++;
    }
    free(line);
    fclose(fp);

    qsort(arr, cnt, sizeof(idx_entry_t), cmp_entry);

    char idx_path[4096];
    derive_idx_path(csv_path, idx_path, sizeof(idx_path));
    if (idx_path[0] == '\0')
    {
        fprintf(stderr, "CSV 路径过长，无法推导索引路径\n");
        free(arr);
        return 1;
    }

    FILE *out = fopen(idx_path, "wb");
    if (out == NULL)
    {
        perror("创建索引文件失败");
        free(arr);
        return 1;
    }
    if (cnt > 0 && fwrite(arr, sizeof(idx_entry_t), cnt, out) != cnt)
    {
        perror("写入索引文件失败");
        fclose(out);
        free(arr);
        return 1;
    }
    fclose(out);
    free(arr);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "已创建索引: %s (共 %zu 条, 每条 16 字节: id int64 + offset int64, 已按 id 排序)\n",
            idx_path, cnt);
    fprintf(stderr, "建索引用时: %.3f ms\n", elapsed_ms);
    return 0;
}

int idx_lookup(const char *csv_path, int64_t qid, int64_t *offset_out)
{
    char idx_path[4096];
    derive_idx_path(csv_path, idx_path, sizeof(idx_path));
    FILE *idx = (idx_path[0] != '\0') ? fopen(idx_path, "rb") : NULL;
    if (idx == NULL)
    {
        return -1; /* 无索引文件：调用方回退全表扫描 */
    }

    if (fseeko(idx, 0, SEEK_END) != 0)
    {
        perror("索引 seek 失败");
        fclose(idx);
        return -2;
    }
    long long size = ftello(idx);
    if (size < 0 || size % (long long)sizeof(idx_entry_t) != 0)
    {
        fprintf(stderr, "索引文件损坏: %s\n", idx_path);
        fclose(idx);
        return -2;
    }
    size_t cnt = (size_t)(size / (long long)sizeof(idx_entry_t));
    if (fseeko(idx, 0, SEEK_SET) != 0)
    {
        perror("索引 seek 失败");
        fclose(idx);
        return -2;
    }
    idx_entry_t *arr = NULL;
    if (cnt > 0)
    {
        arr = (idx_entry_t *)malloc(size);
        if (arr == NULL)
        {
            fprintf(stderr, "内存分配失败\n");
            fclose(idx);
            return -2;
        }
        if (fread(arr, sizeof(idx_entry_t), cnt, idx) != cnt)
        {
            perror("读取索引文件失败");
            free(arr);
            fclose(idx);
            return -2;
        }
    }
    fclose(idx);

    /* 二分查找（索引已按 id 排序） */
    size_t lo = 0, hi = cnt;
    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        if (arr[mid].id < qid)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    int found = (lo < cnt && arr[lo].id == qid);
    if (found)
    {
        *offset_out = arr[lo].offset;
    }
    free(arr);
    return found ? 1 : 0;
}
