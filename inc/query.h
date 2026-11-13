#ifndef QUERY_H
#define QUERY_H

#include <time.h>

int query_db(const char *db_path, const char *id_str);

int query_scan(const char *db_path, const char *id_str, struct timespec t0);

#endif
