#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "import.h"
#include "insert.h"
#include "index.h"
#include "query.h"
#include "delete.h"
#include "update.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        /* 导入CSV数据 */
        fprintf(stderr, "用法: %s <csv文件> import <输出.dbd>\n", argv[0]);
        fprintf(stderr, "示例: %s students.csv import students.dbd\n", argv[0]);
        /* 创建索引 */
        fprintf(stderr, "用法: %s <数据.dbd> index\n", argv[0]);
        /* 查询数据 */
        fprintf(stderr, "用法: %s <数据.dbd> <ID>\n", argv[0]);
        fprintf(stderr, "示例: %s students.dbd 888888\n", argv[0]);
        /* 更新数据 */
        fprintf(stderr, "用法: %s <数据.dbd> update <ID> <列> <值>\n", argv[0]);
        fprintf(stderr, "示例: %s students.dbd update 1 name 张三\n", argv[0]);
        /* 删除数据 */
        fprintf(stderr, "用法: %s <数据.dbd> delete <ID>\n", argv[0]);
        fprintf(stderr, "示例: %s students.dbd delete 1\n", argv[0]);
        /* 插入数据 */
        fprintf(stderr, "用法: %s <数据.dbd> insert <CSV行>\n", argv[0]);
        fprintf(stderr, "示例: %s students.dbd insert \"4,S004,赵六,男,20,计算机,大一,数学,91,zl@x.com,13800000004\"\n", argv[0]);
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
    if (strcmp(argv[2], "index") == 0)
    {
        return build_index(argv[1]);
    }
    if (strcmp(argv[2], "update") == 0)
    {
        if (argc != 6)
        {
            fprintf(stderr, "用法: %s <数据.dbd> update <ID> <列> <值>\n", argv[0]);
            return 2;
        }
        return update_db_field(argv[1], argv[3], argv[4], argv[5]);
    }
    if (strcmp(argv[2], "delete") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "用法: %s <数据.dbd> delete <ID>\n", argv[0]);
            return 2;
        }
        return delete_db_row(argv[1], argv[3]);
    }
    if (strcmp(argv[2], "insert") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "用法: %s <数据.dbd> insert <CSV行>\n", argv[0]);
            return 2;
        }
        return insert_db_row(argv[1], argv[3]);
    }
    return query_db(argv[1], argv[2]);
}
