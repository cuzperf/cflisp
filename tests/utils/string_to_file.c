#include "test_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * 将字符串写入文件
 * @param filename  文件路径
 * @param content   要写入的字符串
 * @param append    0=覆盖写入, 1=追加写入
 * @return          0 成功, -1 失败
 */
int string_to_file(const char* filename, const char* content, int append) {
    if (filename == NULL || content == NULL) {
        return -1;
    }

    const char* mode = append ? "a" : "w";
    FILE* fp = fopen(filename, mode);
    if (fp == NULL) {
        perror("fopen failed");
        return -1;
    }

    // 写入内容
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, fp);

    // 可选：追加换行
    // fputc('\n', fp);

    if (fclose(fp) != 0) {
        perror("fclose failed");
        return -1;
    }

    return (written == len) ? 0 : -1;
}

int delete_file(const char* filename) {
    if (filename == NULL) {
        return -1;
    }

    if (remove(filename) != 0) {
        perror("remove failed");
        return -1;
    }

    return 0;
}
