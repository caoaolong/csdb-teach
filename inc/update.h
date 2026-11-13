#ifndef UPDATE_H
#define UPDATE_H

int update_db_field(const char *db_path, const char *id_str,
                    const char *column, const char *value);

#endif
