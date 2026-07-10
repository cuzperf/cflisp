#include "lisp_internal.h"

// LCOV_EXCL_START

void dump_symtab(Symbol* s)
{
    printf("%s :", s->name);
    cf_print(s->binding);
    NL;
    if (s->left) {
        printf(" ");
        dump_symtab(s->left);
    }
    if (s->right) {
        printf(" ");
        dump_symtab(s->right);
    }
}

void dump_heap()
{
    memory_t tmp = g_heap;
    while (tmp < g_curheap - sizeof(List)) {
        value_t val = ((List*)tmp)->head;
        cf_print(val);
        printf(" -|- ");
        tmp += sizeof(List);
    }
}

void dump_stack()
{
    int cur = 0;
    while (cur < g_sp) {
        cf_print(g_stack[cur]);
        cur++;
        printf("\t");
    }
    printf("\n");
}

void dump_env()
{
    int cur = 0;
    while (cur < g_env_sp) {
        cf_print(g_env_stack[cur]);
        cur++;
        printf("\t");
    }
    printf("\n");
}

// LCOV_EXCL_STOP
