#include "lib/str.h"

int strendswith(const char *str, const char *suffix) {
    // 获取字符串的长度
    size_t strLen = strlen(str);
    size_t suffixLen = strlen(suffix);

    // 如果后缀比字符串长，返回 0
    if (suffixLen > strLen) {
        return 0;
    }

    // 比较字符串末尾的部分
    return strcmp(str + strLen - suffixLen, suffix) == 0;
}