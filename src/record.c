#include <string.h>

#include "engine.h"
#include "record.h"

_Static_assert(sizeof(db_header_t) == 32, "db_header_t must be 32 bytes");
_Static_assert(sizeof(student_t) == 280, "student_t must be 280 bytes");

long long db_row_offset(uint64_t rid, uint32_t row_size)
{
    return (long long)(sizeof(db_header_t) + rid * (uint64_t)row_size);
}

int db_read_header(FILE *fp, db_header_t *out)
{
    if (fread(out, sizeof(*out), 1, fp) != 1)
    {
        return -1;
    }
    if (memcmp(out->magic, DB_MAGIC, DB_MAGIC_LEN) != 0 || out->version != DB_VERSION)
    {
        return -1;
    }
    return 0;
}

static void print_esc(const char *s)
{
    char e[256];
    csv_escape(s, e, sizeof(e));
    fputs(e, stdout);
}

void db_print_row(const student_t *r)
{
    printf("%lld,", (long long)r->id);
    print_esc(r->student_no);
    putchar(',');
    print_esc(r->name);
    putchar(',');
    print_esc(r->gender);
    printf(",%d,", (int)r->age);
    print_esc(r->major);
    putchar(',');
    print_esc(r->grade);
    putchar(',');
    print_esc(r->course);
    printf(",%d,", (int)r->score);
    print_esc(r->email);
    putchar(',');
    print_esc(r->phone);
    putchar('\n');
}
