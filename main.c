#define _POSIX_C_SOURCE 200809L /* clock_gettime 需要 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 解析一行 CSV 的首个字段（处理引号包裹与 "" 转义），结果写入 out */
static void first_field(const char *line, char *out, size_t cap)
{
    size_t n = 0;
    if (line[0] == '"')
    {
        line++; /* 跳过开引号 */
        while (*line != '\0' && n + 1 < cap)
        {
            if (*line == '"')
            {
                if (*(line + 1) == '"')
                {
                    out[n++] = '"';
                    line += 2;
                }
                else
                {
                    break; /* 闭引号，后面应为分隔符或行尾 */
                }
            }
            else if (*line == ',' || *line == '\r' || *line == '\n')
            {
                break;
            }
            else
            {
                out[n++] = *line++;
            }
        }
    }
    else
    {
        while (*line != '\0' && *line != ',' && *line != '\r' && *line != '\n' && n + 1 < cap)
        {
            out[n++] = *line++;
        }
    }
    out[n] = '\0';
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "用法: %s <csv文件> <ID>\n", argv[0]);
        fprintf(stderr, "示例: %s students.csv 3\n", argv[0]);
        return 2;
    }

    FILE *fp = fopen(argv[1], "r");
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
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        lineno++;
        const char *p = line;
        if (lineno == 1 && nread >= 3 &&
            (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB &&
            (unsigned char)p[2] == 0xBF)
        {
            p += 3; /* 跳过 UTF-8 BOM（csdb-gd 默认 utf8-bom 编码） */
        }
        first_field(p, field, sizeof(field));
        if (strcmp(field, argv[2]) == 0)
        {
            fputs(line, stdout);
            found = 1;
            break;
        }
    }

    free(line);
    fclose(fp);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 +
                        (t1.tv_nsec - t0.tv_nsec) / 1e6;
    fprintf(stderr, "查询用时: %.3f ms\n", elapsed_ms);

    if (!found)
    {
        fprintf(stderr, "未找到 ID 为 %s 的行\n", argv[2]);
        return 1;
    }
    return 0;
}
