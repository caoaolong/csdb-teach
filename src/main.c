#include <stdlib.h>
#include "csdb/db.h"
#include "db_file.h"
#include "db_file_block.h"
#include "db_row.h"
#include "lib/code.h"

void db_init()
{
    db_file_init();
}

int main(int argc, char *argv[])
{
    db_init();
    db_file_t *db_file = db_file_open("test");

    db_schema_row_t *user = db_schema_load_table(db_file, "user");
    printf("table(%s, comment=%s)\n", user->name, (char *)user->comment);

    db_schema_row_t *pr = user;
    data_ref_t ref;
    while (pr->next) {
        ref_decode(pr->next, &ref);
        pr = db_schema_load_column(db_file, ref.page, ref.offset);
        printf("  column(%s, comment=%s)\n", pr->name, (char *)pr->comment);
    }

    // db_init();

    // // 创建数据库
    // db_file_create("test");

    // // 创建表
    // db_file_t *db_file = db_file_open("test");

    // db_schema_row_t *user = db_schema_new("user", "this is user table comment!", ROW_TABLE);
    // db_file_write_schema(db_file, user, false);

    // db_schema_row_t *id = db_schema_new("id", "user id", ROW_COLUMN);
    // db_file_write_schema(db_file, id, false);

    // db_schema_link(user, id);
    // db_file_link_schema(db_file, user, false);

    // db_schema_row_t *name = db_schema_new("name", "user name", ROW_COLUMN);
    // db_file_write_schema(db_file, name, false);
    
    // db_schema_link(id, name);
    // db_file_link_schema(db_file, id, false);

    // db_file_commit(db_file);

    return 0;
}