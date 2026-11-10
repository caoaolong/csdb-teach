#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "import.h"
#include "query.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "用法: %s <csv文件> import <输出.dbd>\n", argv[0]);
        fprintf(stderr, "示例: %s students.csv import students.dbd\n", argv[0]);
        fprintf(stderr, "用法: %s <数据.dbd> <ID>\n", argv[0]);
        fprintf(stderr, "示例: %s students.dbd 888888\n", argv[0]);
        return 2;
    }

    if (strcmp(argv[2], "import") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "用法: %s <csv文件> import <输出.dbd>\n", argv[0]);
            return 2;
        }
        return import_csv(argv[1], argv[3]);
    }
    return query_db(argv[1], argv[2]);
}
