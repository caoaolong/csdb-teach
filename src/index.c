#define _POSIX_C_SOURCE 200809L /* fseeko / ftello 需要 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "engine.h"
#include "index.h"
#include "record.h"

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

void derive_dbi_path(const char *db_path, char *out, size_t cap)
{
    const char *slash = strrchr(db_path, '/');
    const char *backslash = strrchr(db_path, '\\');
    const char *dir_end = slash;
    if (backslash != NULL && (dir_end == NULL || backslash > dir_end))
    {
        dir_end = backslash;
    }
    const char *base = (dir_end != NULL) ? dir_end + 1 : db_path;
    size_t dir_len = (dir_end != NULL) ? (size_t)(dir_end - db_path + 1) : 0;

    const char *dot = strrchr(base, '.');
    size_t base_len = (dot != NULL) ? (size_t)(dot - base) : strlen(base);

    /* dir + base(去后缀) + ".dbi" */
    size_t need = dir_len + base_len + 4 + 1;
    if (need > cap)
    {
        if (cap > 0)
        {
            out[0] = '\0';
        }
        return;
    }
    memcpy(out, db_path, dir_len);
    memcpy(out + dir_len, base, base_len);
    memcpy(out + dir_len + base_len, ".dbi", 5); /* 含 NUL */
}

static void idx_fill_header(idx_header_t *h, int64_t db_size, int64_t db_mtime, uint64_t count)
{
    memcpy(h->magic, IDX_MAGIC, IDX_MAGIC_LEN);
    h->db_size = db_size;
    h->db_mtime = db_mtime;
    h->count = count;
}

int build_index(const char *db_path)
{
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    struct stat st;
    int64_t src_size = -1, src_mtime = -1;
    if (stat(db_path, &st) == 0)
    {
        src_size = (int64_t)st.st_size;
        src_mtime = (int64_t)st.st_mtime;
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

    /* 条目数上确界就是行数，一次分配，无需扩容 */
    size_t cnt = 0;
    idx_entry_t *arr = NULL;
    if (hdr.row_count > 0)
    {
        arr = (idx_entry_t *)malloc((size_t)hdr.row_count * sizeof(idx_entry_t));
        if (arr == NULL)
        {
            fprintf(stderr, "内存分配失败\n");
            fclose(fp);
            return 1;
        }
    }

    student_t r;
    for (uint64_t i = 0; i < hdr.row_count; i++)
    {
        long long off = (long long)(sizeof(db_header_t) + i * (uint64_t)sizeof(student_t));
        if (fread(&r, sizeof(r), 1, fp) != 1)
        {
            perror("读取记录失败");
            free(arr);
            fclose(fp);
            return 1;
        }
        arr[cnt].id = r.id;
        arr[cnt].offset = (int64_t)off;
        cnt++;
    }
    fclose(fp);

    qsort(arr, cnt, sizeof(idx_entry_t), cmp_entry);

    char idx_path[PATH_CAP];
    derive_dbi_path(db_path, idx_path, sizeof(idx_path));
    if (idx_path[0] == '\0')
    {
        fprintf(stderr, "路径过长，无法推导索引路径\n");
        free(arr);
        return 1;
    }
    char tmp_path[TMP_CAP];
    if (make_tmp_path(idx_path, tmp_path, sizeof(tmp_path)) != 0)
    {
        fprintf(stderr, "路径过长\n");
        free(arr);
        return 1;
    }

    FILE *out = fopen(tmp_path, "wb");
    if (out == NULL)
    {
        perror("创建索引文件失败");
        free(arr);
        return 1;
    }
    idx_header_t ihdr;
    idx_fill_header(&ihdr, src_size, src_mtime, (uint64_t)cnt);
    if (fwrite(&ihdr, sizeof(ihdr), 1, out) != 1)
    {
        perror("写入索引头失败");
        fclose(out);
        remove(tmp_path);
        free(arr);
        return 1;
    }
    if (cnt > 0 && fwrite(arr, sizeof(idx_entry_t), cnt, out) != cnt)
    {
        perror("写入索引文件失败");
        fclose(out);
        remove(tmp_path);
        free(arr);
        return 1;
    }
    fflush(out);
    if (file_fsync(out) != 0)
    {
        perror("索引 fsync 失败(继续尝试替换)");
    }
    fclose(out);
    if (file_replace(tmp_path, idx_path) != 0)
    {
        perror("索引原子替换失败");
        remove(tmp_path);
        free(arr);
        return 1;
    }
    free(arr);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "已创建索引: %s (共 %zu 条)\n", idx_path, cnt);
    fprintf(stderr, "建索引用时: %.3f ms\n", elapsed_ms);
    return 0;
}

int idx_lookup(const char *db_path, int64_t qid, int64_t *offset_out)
{
    char idx_path[PATH_CAP];
    derive_dbi_path(db_path, idx_path, sizeof(idx_path));
    FILE *idx = (idx_path[0] != '\0') ? fopen(idx_path, "rb") : NULL;
    if (idx == NULL)
    {
        return -1; /* 无索引文件：调用方重建或回退扫描 */
    }

    if (fseeko(idx, 0, SEEK_END) != 0)
    {
        perror("索引 seek 失败");
        fclose(idx);
        return -2;
    }
    long long size = ftello(idx);
    if (size < (long long)sizeof(idx_header_t) ||
        (size - (long long)sizeof(idx_header_t)) % (long long)sizeof(idx_entry_t) != 0)
    {
        fprintf(stderr, "索引文件损坏，请重建: %s\n", idx_path);
        fclose(idx);
        return -2;
    }
    if (fseeko(idx, 0, SEEK_SET) != 0)
    {
        perror("索引 seek 失败");
        fclose(idx);
        return -2;
    }
    idx_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, idx) != 1)
    {
        perror("读取索引头失败");
        fclose(idx);
        return -2;
    }
    if (memcmp(hdr.magic, IDX_MAGIC, IDX_MAGIC_LEN) != 0)
    {
        fprintf(stderr, "索引文件损坏，请重建: %s\n", idx_path);
        fclose(idx);
        return -2;
    }
    size_t cnt = (size_t)hdr.count;
    if ((long long)sizeof(idx_header_t) + (long long)cnt * (long long)sizeof(idx_entry_t) != size)
    {
        fprintf(stderr, "索引文件损坏: %s\n", idx_path);
        fclose(idx);
        return -2;
    }

    /* 过期检测：数据文件变化后索引不可信 */
    struct stat st;
    if (stat(db_path, &st) == 0)
    {
        if ((int64_t)st.st_size != hdr.db_size || (int64_t)st.st_mtime != hdr.db_mtime)
        {
            fclose(idx);
            return -1;
        }
    }

    idx_entry_t *arr = NULL;
    if (cnt > 0)
    {
        arr = (idx_entry_t *)malloc(cnt * sizeof(idx_entry_t));
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

int idx_refresh(const char *db_path)
{
    char idx_path[PATH_CAP];
    derive_dbi_path(db_path, idx_path, sizeof(idx_path));
    if (idx_path[0] == '\0')
    {
        fprintf(stderr, "路径过长，无法推导索引路径\n");
        return 1;
    }

    struct stat st;
    if (stat(db_path, &st) != 0)
    {
        perror("stat 数据文件失败");
        return 1;
    }

    FILE *idx = fopen(idx_path, "r+b");
    if (idx == NULL)
    {
        return 1; /* 无索引可刷：调用方决定是否重建 */
    }
    idx_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, idx) != 1 ||
        memcmp(hdr.magic, IDX_MAGIC, IDX_MAGIC_LEN) != 0)
    {
        fprintf(stderr, "索引文件损坏，请重建: %s\n", idx_path);
        fclose(idx);
        return 1;
    }
    idx_fill_header(&hdr, (int64_t)st.st_size, (int64_t)st.st_mtime, hdr.count);
    if (fseeko(idx, 0, SEEK_SET) != 0 || fwrite(&hdr, sizeof(hdr), 1, idx) != 1)
    {
        perror("刷新索引头失败");
        fclose(idx);
        return 1;
    }
    fflush(idx);
    if (file_fsync(idx) != 0)
    {
        perror("索引 fsync 失败");
        fclose(idx);
        return 1;
    }
    fclose(idx);
    return 0;
}
