#ifndef ENGINE_H
#define ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 路径缓冲：4096 为常见 PATH_MAX；TMP 多留 8 字节给 ".tmp" 后缀 + NUL */
#define PATH_CAP 4096
#define TMP_CAP (PATH_CAP + 8)

/* CSV：按列取原始跨度 / 去转义值 / RFC4180 转义 */
int csv_span(const char *line, size_t len, size_t target, size_t *s_out, size_t *e_out);
int csv_value(const char *line, size_t len, size_t target, char *out, size_t cap);
void csv_escape(const char *v, char *out, size_t cap);

/* ID：严格解析纯数字 */
int parse_id(const char *field, int64_t *out);

/* 首行 BOM 跳过：非首行或无 BOM 返回原指针 */
const char *skip_bom(const char *p, size_t nread, int is_first);

int file_fsync(FILE *fp);
int file_replace(const char *tmp_path, const char *dst_path);
int make_tmp_path(const char *src, char *out, size_t cap);

#endif
