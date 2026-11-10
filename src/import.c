#define _POSIX_C_SOURCE 200809L /* getline / fseeko 需要 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"
#include "import.h"
#include "record.h"

/* 期望列（按 CSV 列序无关，靠表头名定位） */
static const char *WANT[] = {
    "id", "student_no", "name", "gender", "age",
    "major", "grade", "course", "score", "email", "phone",
};
#define NCOL (sizeof(WANT) / sizeof(WANT[0]))

static void set_text(char *dst, size_t cap, const char *v)
{
    strncpy(dst, v, cap - 1);
    dst[cap - 1] = '\0';
}

static int32_t to_i32(const char *v)
{
    char *end = NULL;
    long x = strtol(v, &end, 10);
    if (end == v || *end != '\0')
    {
        return 0; /* 脏数据归零，import 只保证不断行 */
    }
    return (int32_t)x;
}

int import_csv(const char *csv_path, const char *db_path)
{
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    FILE *fp = fopen(csv_path, "rb");
    if (!fp)
    {
        perror("打开文件失败");
        return 1;
    }

    char tmp_path[TMP_CAP];
    if (make_tmp_path(db_path, tmp_path, sizeof(tmp_path)) != 0)
    {
        fprintf(stderr, "路径过长\n");
        fclose(fp);
        return 1;
    }
    FILE *out = fopen(tmp_path, "wb");
    if (!out)
    {
        perror("创建临时文件失败");
        fclose(fp);
        return 1;
    }

    /* 先占位头，行数扫完再回填 */
    db_header_t hdr;
    memcpy(hdr.magic, DB_MAGIC, DB_MAGIC_LEN);
    hdr.version = DB_VERSION;
    hdr.row_size = (uint32_t)sizeof(student_t);
    hdr.row_count = 0;
    hdr.reserved = 0;
    if (fwrite(&hdr, sizeof(hdr), 1, out) != 1)
    {
        perror("写入文件头失败");
        fclose(fp);
        fclose(out);
        remove(tmp_path);
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    long lineno = 0;
    size_t col[NCOL];
    int have_col = 0;
    uint64_t count = 0;
    char v[256];

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        lineno++;
        size_t llen = (size_t)nread;
        const char *body = skip_bom(line, llen, lineno == 1);
        llen -= (size_t)(body - line);

        if (lineno == 1)
        {
            /* 表头定位每一列 */
            for (size_t c = 0; c < NCOL; c++)
            {
                col[c] = (size_t)-1;
            }
            for (size_t c = 0;; c++)
            {
                if (!csv_value(body, llen, c, v, sizeof(v)))
                {
                    break;
                }
                for (size_t k = 0; k < NCOL; k++)
                {
                    if (strcmp(v, WANT[k]) == 0)
                    {
                        col[k] = c;
                    }
                }
            }
            for (size_t k = 0; k < NCOL; k++)
            {
                if (col[k] == (size_t)-1)
                {
                    fprintf(stderr, "表头缺少列: %s\n", WANT[k]);
                    free(line);
                    fclose(fp);
                    fclose(out);
                    remove(tmp_path);
                    return 1;
                }
            }
            have_col = 1;
            continue;
        }
        if (!have_col || llen == 0 || body[0] == '\r' || body[0] == '\n')
        {
            continue; /* 空行跳过 */
        }
        if (!csv_value(body, llen, col[0], v, sizeof(v)))
        {
            continue;
        }
        int64_t id = 0;
        if (!parse_id(v, &id))
        {
            continue; /* 跳过非数字 ID 行，与建索引一致 */
        }

        student_t r;
        memset(&r, 0, sizeof(r));
        r.id = id;
        if (csv_value(body, llen, col[1], v, sizeof(v)))
        {
            set_text(r.student_no, sizeof(r.student_no), v);
        }
        if (csv_value(body, llen, col[2], v, sizeof(v)))
        {
            set_text(r.name, sizeof(r.name), v);
        }
        if (csv_value(body, llen, col[3], v, sizeof(v)))
        {
            set_text(r.gender, sizeof(r.gender), v);
        }
        if (csv_value(body, llen, col[4], v, sizeof(v)))
        {
            r.age = to_i32(v);
        }
        if (csv_value(body, llen, col[5], v, sizeof(v)))
        {
            set_text(r.major, sizeof(r.major), v);
        }
        if (csv_value(body, llen, col[6], v, sizeof(v)))
        {
            set_text(r.grade, sizeof(r.grade), v);
        }
        if (csv_value(body, llen, col[7], v, sizeof(v)))
        {
            set_text(r.course, sizeof(r.course), v);
        }
        if (csv_value(body, llen, col[8], v, sizeof(v)))
        {
            r.score = to_i32(v);
        }
        if (csv_value(body, llen, col[9], v, sizeof(v)))
        {
            set_text(r.email, sizeof(r.email), v);
        }
        if (csv_value(body, llen, col[10], v, sizeof(v)))
        {
            set_text(r.phone, sizeof(r.phone), v);
        }
        if (fwrite(&r, sizeof(r), 1, out) != 1)
        {
            perror("写入记录失败");
            free(line);
            fclose(fp);
            fclose(out);
            remove(tmp_path);
            return 1;
        }
        count++;
    }
    free(line);
    fclose(fp);

    /* 回填行数 */
    hdr.row_count = count;
    if (fseeko(out, 0, SEEK_SET) != 0 || fwrite(&hdr, sizeof(hdr), 1, out) != 1)
    {
        perror("回填文件头失败");
        fclose(out);
        remove(tmp_path);
        return 1;
    }
    fflush(out);
    file_fsync(out);
    fclose(out);

    if (file_replace(tmp_path, db_path) != 0)
    {
        perror("原子替换失败");
        remove(tmp_path);
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "已导入: %s (共 %llu 行, 每行 %u 字节)\n",
            db_path, (unsigned long long)count, (unsigned)sizeof(student_t));
    fprintf(stderr, "导入用时: %.3f ms\n", elapsed_ms);
    return 0;
}
