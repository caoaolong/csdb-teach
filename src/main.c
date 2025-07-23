#include <stdlib.h>
#include "csdb/db.h"
#include "db_file.h"

void db_init()
{
    db_file_init();
}

int main(int argc, char *argv[]) 
{
    db_init();
 
    // 创建数据库
    // db_file_create("test");

    db_file_t *db_file = db_file_open("test");

    // 写入数据
    FILE *file = fopen("/usr/bin/lcf", "r");
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(size);
    fread(data, 1, size, file);
    fclose(file);
    db_file_write(db_file, data, size, true);

    return 0;
}