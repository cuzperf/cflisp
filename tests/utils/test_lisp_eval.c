#include "test_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// NOTE: 属于曲线完成 lisp 对字符串的处理了 [陈智鹏@2026-7-3]
value_t cl_eval_string(const char* lispstr)
{
    srand((unsigned)time(NULL));
    char filename[32];
    int r1 = rand() % 100000;   // Mac 平台 rand 值域更大
    int r2 = rand() % 100000;   // 限制最大长度为 5
    sprintf(filename, "build/test_%d_%d.lsp", r1, r2);

    string_to_file(filename, lispstr, 0);

    value_t sexp = read_file(filename);
    value_t res = eval_toplevel(sexp);

    delete_file(filename);

    return res;
}
