#include "lisp_internal.h"

#include <setjmp.h>

// LCOV_EXCL_START

// for error handling in REPL
static jmp_buf jmp_mark;
static bool in_repl = false;

void fail()
{
    if (in_repl) {
        longjmp(jmp_mark, -1);
    } else {
        exit(1);
    }
}

CF_API void cf_lisp_repl()
{
    in_repl = true;
    while (true) {
        NL;
        printf(">");
        int ss = g_sp;
        int ee = g_env_sp;

        if (!setjmp(jmp_mark)) {
            read(stdin, &symtab);
            if (ss != g_sp) {
                value_t res = eval(pop());
                cf_print(res);
            }
        } else {
            restore_stack(ss);
            env_restore_stack(ee);
        }
    }
}

// LCOV_EXCL_STOP
