#ifndef _TEST_UTILS_H_
#define _TEST_UTILS_H_

#include "lisp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 将字符串写入文件
 * @param filename  文件路径
 * @param content   要写入的字符串
 * @param append    0=覆盖写入, 1=追加写入
 * @return          0 成功, -1 失败
 */
int string_to_file(const char* filename, const char* content, int append);

/**
 * @brief 删除文件
 * @param filename  文件路径
 * @return          0 成功, -1 失败
 */
int delete_file(const char* filename);


/**
 * @brief 执行字符串中的语句
 * @param lispstr lisp 语法的字符串
 * @return 语句结果
 */
value_t cl_eval_string(const char* lispstr);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_UTILS_H_ */
