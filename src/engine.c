#define _POSIX_C_SOURCE 200809L /* fileno 需要 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "engine.h"

int csv_span(const char *line, size_t len, size_t target, size_t *s_out, size_t *e_out)
{
    size_t i = 0, idx = 0;
    for (;;)
    {
        size_t rs = i;
        if (i < len && line[i] == '"')
        {
            i++; /* 开引号 */
            while (i < len)
            {
                if (line[i] == '"')
                {
                    if (i + 1 < len && line[i + 1] == '"')
                    {
                        i += 2; /* 转义 "" */
                    }
                    else
                    {
                        i++; /* 闭引号 */
                        break;
                    }
                }
                else
                {
                    i++;
                }
            }
        }
        else
        {
            while (i < len && line[i] != ',' && line[i] != '\r' && line[i] != '\n')
            {
                i++;
            }
        }
        if (idx == target)
        {
            *s_out = rs;
            *e_out = i;
            return 1;
        }
        idx++;
        if (i < len && line[i] == ',')
        {
            i++; /* 下一列 */
            continue;
        }
        return 0; /* 列数不够 */
    }
}

int csv_value(const char *line, size_t len, size_t target, char *out, size_t cap)
{
    size_t rs, re;
    if (!csv_span(line, len, target, &rs, &re))
    {
        return 0;
    }
    size_t n = 0;
    if (rs < re && line[rs] == '"')
    {
        size_t i = rs + 1;
        size_t end = re;
        if (end > rs && line[end - 1] == '"')
        {
            end--; /* 去闭引号 */
        }
        while (i < end && n + 1 < cap)
        {
            if (line[i] == '"' && i + 1 < end && line[i + 1] == '"')
            {
                out[n++] = '"';
                i += 2;
            }
            else
            {
                out[n++] = line[i++];
            }
        }
    }
    else
    {
        size_t k = rs;
        while (k < re && n + 1 < cap)
        {
            out[n++] = line[k++];
        }
    }
    out[n] = '\0';
    return 1;
}

void csv_escape(const char *v, char *out, size_t cap)
{
    int need_q = 0;
    for (const char *p = v; *p; p++)
    {
        if (*p == ',' || *p == '"' || *p == '\r' || *p == '\n')
        {
            need_q = 1;
            break;
        }
    }
    size_t n = 0;
    if (!need_q)
    {
        while (*v && n + 1 < cap)
        {
            out[n++] = *v++;
        }
        out[n] = '\0';
        return;
    }
    if (n + 1 < cap)
    {
        out[n++] = '"';
    }
    while (*v && n + 1 < cap)
    {
        if (*v == '"')
        {
            if (n + 2 < cap)
            {
                out[n++] = '"';
                out[n++] = '"';
            }
            else
            {
                break;
            }
        }
        else
        {
            out[n++] = *v;
        }
        v++;
    }
    if (n + 1 < cap)
    {
        out[n++] = '"';
    }
    out[n] = '\0';
}

int parse_id(const char *field, int64_t *out)
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

const char *skip_bom(const char *p, size_t nread, int is_first)
{
    if (is_first && nread >= 3 &&
        (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB &&
        (unsigned char)p[2] == 0xBF)
    {
        return p + 3; /* 跳过 UTF-8 BOM（csdb-gd 默认 utf8-bom 编码） */
    }
    return p;
}

int file_fsync(FILE *fp)
{
#ifdef _WIN32
    return _commit(_fileno(fp));
#else
    return fsync(fileno(fp));
#endif
}

int file_replace(const char *tmp_path, const char *dst_path)
{
#ifdef _WIN32
    remove(dst_path); /* Windows 下 rename 不覆盖已存在文件，先删 */
#endif
    if (rename(tmp_path, dst_path) != 0)
    {
        return -1;
    }
    return 0;
}

int make_tmp_path(const char *src, char *out, size_t cap)
{
    if (snprintf(out, cap, "%s.tmp", src) >= (int)cap)
    {
        return -1;
    }
    return 0;
}
