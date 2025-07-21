#include "csdb/db.h"
#include "db_file.h"

void db_init()
{
    db_file_init();
}

int main(int argc, char *argv[]) 
{
    db_init();
    
    db_file_create("test");
    return 0;
}