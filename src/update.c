#define _POSIX_C_SOURCE 200809L /* getline 需要 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "index.h"
#include "update.h"

int update_student_no(const char *csv_path, const char *id_str, const char *new_sn)
{
    if (!id_str || !id_str[0] || !new_sn)
    {
        fprintf(stderr, "参数错误：ID 与新学号不能为空\n");
        return 1;
    }
    for (const char *p = new_sn; *p; p++)
    {
        if (*p < '0' || *p > '9')
        {
            fprintf(stderr, "新学号必须为纯数字\n");
            return 1;
        }
    }

    FILE *fp = fopen(csv_path, "rb");
    if (!fp)
    {
        perror("打开文件失败");
        return 1;
    }

    char tmp_path[TMP_CAP];
    if (make_tmp_path(csv_path, tmp_path, sizeof(tmp_path)) != 0)
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

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    long lineno = 0;
    size_t sn_col = 0;
    int have_col = 0, found = 0, bad = 0;

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        lineno++;
        size_t llen = (size_t)nread;
        const char *body = skip_bom(line, llen, lineno == 1);
        size_t body_off = (size_t)(body - line);
        llen -= body_off;

        if (lineno == 1)
        {
            /* 表头定位 student_no 列（大小写敏感），找不到则报错 */
            char v[256];
            size_t c = 0;
            for (;; c++)
            {
                if (!csv_value(body, llen, c, v, sizeof(v)))
                {
                    break;
                }
                if (strcmp(v, "student_no") == 0)
                {
                    sn_col = c;
                    have_col = 1;
                    break;
                }
            }
            if (!have_col)
            {
                fprintf(stderr, "表头缺少 student_no 列\n");
                bad = 1;
                break;
            }
            fwrite(line, 1, (size_t)nread, out);
            continue;
        }

        /* 空行原样拷贝 */
        if (llen == 0 || body[0] == '\r' || body[0] == '\n' || body[0] == '\0')
        {
            fwrite(line, 1, (size_t)nread, out);
            continue;
        }

        char f0[1024];
        if (!csv_value(body, llen, 0, f0, sizeof(f0)))
        {
            fwrite(line, 1, (size_t)nread, out);
            continue;
        }
        if (!found && id_match(f0, id_str))
        {
            size_t rs, re;
            if (!csv_span(body, llen, sn_col, &rs, &re))
            {
                fprintf(stderr, "ID 为 %s 的行列数不足，无法更新\n", id_str);
                bad = 1;
                break;
            }
            rs += body_off;
            re += body_off;
            size_t wlen = (size_t)nread;
            fwrite(line, 1, rs, out);
            fwrite(new_sn, 1, strlen(new_sn), out);
            fwrite(line + re, 1, wlen - re, out);
            found = 1;
        }
        else
        {
            fwrite(line, 1, (size_t)nread, out);
        }
    }

    free(line);
    fclose(fp);
    if (fflush(out) != 0)
    {
        perror("写入临时文件失败");
        fclose(out);
        remove(tmp_path);
        return 1;
    }
    file_fsync(out);
    fclose(out);

    if (bad)
    {
        remove(tmp_path);
        return 1;
    }
    if (!have_col)
    {
        fprintf(stderr, "空文件或缺少表头\n");
        remove(tmp_path);
        return 1;
    }
    if (!found)
    {
        fprintf(stderr, "未找到 ID 为 %s 的行\n", id_str);
        remove(tmp_path);
        return 1;
    }

    if (file_replace(tmp_path, csv_path) != 0)
    {
        perror("原子替换失败");
        remove(tmp_path);
        return 1;
    }

    /* 数据 mtime/size 已变，旧索引不可信，直接重建 */
    if (build_index(csv_path) != 0)
    {
        fprintf(stderr, "数据已更新，但重建索引失败，请手动执行 index\n");
        return 1;
    }
    fprintf(stderr, "已更新 ID=%s 的 student_no 为 %s\n", id_str, new_sn);
    return 0;
}
