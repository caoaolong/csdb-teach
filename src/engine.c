#include <stddef.h>

#include "engine.h"

void first_field(const char *line, char *out, size_t cap)
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
