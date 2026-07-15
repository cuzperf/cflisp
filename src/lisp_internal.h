#ifndef _LISP_INTERNAL_H_
#define _LISP_INTERNAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include "lisp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t type_t;

#define UNUSED(x) (void)(x)

#define TAG_NUM 0x0
#define TAG_LIST 0x1
#define TAG_SYM 0x2
#define TAG_OTHER 0x3

#define TYPE_NUM 0x0
#define TYPE_LIST 0x1
#define TYPE_SYM 0x2
#define TYPE_BUILTIN 0x3

#define UNBOUND ((value_t) TAG_SYM)
#define EMPTY_LIST ((value_t) TAG_LIST)
#define RELOCATED_MARK ((value_t)0x000101)
#define END RELOCATED_MARK
#define tag(x) ((x)&0x3)
#define ptr(x) ((void*)((x) & ~(value_t)0x3))
#define tagptr(x, t) (((value_t)(x)) | (t))
#define number(x) (((value_t)(x)) << 2)
#define sym_val(x) ((Symbol*)(ptr(x)))
#define list_val(x) ((List *)ptr(x))
#define builtin_val(x) ((Builtin *)ptr(x))
#define is_num(x) (tag(x) == TAG_NUM)
#define is_list(x) (tag(x) == TAG_LIST)
#define is_sym(x) (tag(x) == TAG_SYM)
#define list(x) (tagptr((x), TAG_LIST))
#define type(x) *((type_t*)x)
#define error(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); fail(); } while(0)
#define head(l) (safe_listval(l)->head)
#define tail(l) (safe_listval(l)->tail)
#define head_(l) (list_val(l)->head)
#define tail_(l) (list_val(l)->tail)
#define cf_assert(cond, ...) do { if (!(cond)) error(__VA_ARGS__); } while(0)
// NOTE: # 和 ## 操作符会阻止其操作数的进一步宏展开，嵌套一层是为了让参数在"见到" # 之前先完成自身的宏展开 [陈智鹏@2026-6-27]。
#define str(x) _str(x)
#define _str(x) #x
#define assert_type(val, type) \
    cf_assert(type_of(val) == TYPE_##type, "Error: expected " str(type))
#define NL do { printf("\n"); } while(0)

typedef struct {
    type_t type;
} Type;

typedef struct {
    value_t head;
    value_t tail;
} List;

typedef uint32_t hash_t;
typedef struct _Symbol {
    value_t binding;
    hash_t hash;
    struct _Symbol* left;
    struct _Symbol* right;
    char name[1];
} Symbol;

#define CF_BUILTIN_FUNCTIONS(XX)    \
    XX(B_FN,        "fn")           \
    XX(B_MACRO,     "macro")        \
    XX(B_QUOTE,     "quote")        \
    XX(B_COND,      "cond")         \
    XX(B_DO,        "do")           \
    XX(B_DEF,       "def")          \
    XX(B_AND,       "and")          \
    XX(B_OR,        "or")           \
    XX(F_ADD,       "+")            \
    XX(F_SUB,       "-")            \
    XX(F_DIV,       "/")            \
    XX(F_MUL,       "*")            \
    XX(F_LT,        "<")            \
    XX(F_GT,        ">")            \
    XX(F_EQ,        "=")            \
    XX(F_NOT,       "not")          \
    XX(F_CONS,      "cons")         \
    XX(F_HEAD,      "head")         \
    XX(F_TAIL,      "tail")         \
    XX(F_EVAL,      "eval")         \
    XX(F_APPLY,     "apply")        \
    XX(F_LISTP,     "list?")        \
    XX(F_SYMBOLP,   "symbol?")      \
    XX(F_NUMBERP,   "number?")      \
    XX(F_BUILTINP,  "builtin?")     \
    XX(F_PRINT,     "print")        \
    XX(N_BUILTINS,  "")             \

typedef enum {
#define XX(symbol, name) symbol,
    CF_BUILTIN_FUNCTIONS(XX)
#undef XX
} BuiltinCode;

typedef struct {
    type_t type;
    BuiltinCode code;
} Builtin;

// lisp_gc.c
value_t make_cell(value_t v);
void gc();

// lisp_symbol.c
extern Symbol* symtab;
value_t cons_(value_t h, value_t t);
value_t cons(value_t h, value_t t);
value_t symbol(const char* name, Symbol** env);
void dump_symtab(Symbol* s);

// lisp_list.c
void push_list(value_t l);
value_t pop_list(int ss);
value_t make_list(value_t h, ...);

// lisp_read.c
void read(FILE* f, Symbol** env);

// lisp_print.c
CF_API void cf_(value_t v);
CF_API void cf_m(value_t v);

// lisp_eval.c
value_t eval(value_t);

// lisp_core.c
type_t type_of(value_t v);
void dump_heap();
void dump_stack();
void dump_env();

extern value_t* g_stack;
extern int g_sp;
void push(value_t v);
value_t top();
value_t pop();
value_t popn(int n);
void restore_stack(int n);

extern value_t* g_env_stack;
extern int g_env_sp;
void env_push(value_t v);
value_t env_top();
value_t env_pop();
void env_restore_stack(int n);

extern value_t FN, MACRO;
extern value_t QUOTE, UNQUOTE, QUASIQUOTE, UNQUOTE_SPLICING;
extern value_t REST;
extern value_t NIL, T;

extern const char* builtin_names[N_BUILTINS + 1];

// lisp_repl.c
void fail();

static inline List* safe_listval(value_t v)
{
    if (v == EMPTY_LIST) {
        error("Trying to take head/tail of empty list");
    }
    return list_val(v);
}

#ifdef __cplusplus
}
#endif

#endif /* _LISP_INTERNAL_H_ */
