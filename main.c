#include <stdio.h>
#include <string.h>

#include "index.h"
#include "query.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "用法: %s <csv文件> <ID|index>\n", argv[0]);
        fprintf(stderr, "示例: %s students.csv 3\n", argv[0]);
        fprintf(stderr, "示例: %s students.csv index\n", argv[0]);
        return 2;
    }

    if (strcmp(argv[2], "index") == 0)
    {
        return build_index(argv[1]);
    }
    return query_csv(argv[1], argv[2]);
}
