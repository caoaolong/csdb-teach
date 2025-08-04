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
    return 0;
}