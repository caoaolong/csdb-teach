#ifndef RECORD_H
#define RECORD_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define DB_MAGIC "CSDBD001"
#define DB_MAGIC_LEN 8
#define DB_VERSION 1u

#define SN_LEN 16
#define NAME_LEN 64
#define GENDER_LEN 8
#define MAJOR_LEN 32
#define GRADE_LEN 16
#define COURSE_LEN 32
#define EMAIL_LEN 64
#define PHONE_LEN 32

typedef struct
{
    char magic[DB_MAGIC_LEN];
    uint32_t version;
    uint32_t row_size;
    uint64_t row_count;
    uint64_t reserved;
} db_header_t;

typedef struct
{
    int64_t id;              /* 0 */
    char student_no[SN_LEN]; /* 8 */
    char name[NAME_LEN];     /* 24 */
    char gender[GENDER_LEN]; /* 88 */
    int32_t age;             /* 96 */
    char major[MAJOR_LEN];   /* 100 */
    char grade[GRADE_LEN];   /* 132 */
    char course[COURSE_LEN]; /* 148 */
    int32_t score;           /* 180 */
    char email[EMAIL_LEN];   /* 184 */
    char phone[PHONE_LEN];   /* 248 */
} student_t;                 /* 共 280 字节 */

/* 第 rid 行首字节偏移（rid 从 0 起） */
long long db_row_offset(uint64_t rid, uint32_t row_size);

/* 读并校验文件头，成功返回 0 */
int db_read_header(FILE *fp, db_header_t *out);

/* 按 CSV 列序打印一行（文本列做转义），供 query 输出 */
void db_print_row(const student_t *r);

#endif
