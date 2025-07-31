#include <stdlib.h>
#include "csdb/db.h"
#include "db_file.h"
#include "db_file_block.h"
#include "db_row.h"

void db_init()
{
    db_file_init();
}

int main(int argc, char *argv[])
{
    db_init();
    db_file_t *db_file = db_file_open("test");
    // 读取表
    db_schema_row_t *user = db_schema_load_schema(db_file, "user", ROW_TABLE);
    printf("table: %s, comment=%s\n", user->name, (char *)user->comment);
    return 0;
}