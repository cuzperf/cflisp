#include "lisp_internal.h"

static void print_list(value_t v)
{
    if (v == EMPTY_LIST) {
        printf("()");
        return;
    }

    if (head(v) == QUOTE) {
        printf("'");
        if (tail(v) != EMPTY_LIST) {
            cf_print(head(tail(v)));
        }
        return;
    }

    if (head(v) == QUASIQUOTE) {
        printf("`");
        if (tail(v) != EMPTY_LIST) {
            cf_print(head(tail(v)));
        }
        return;
    }

    if (head(v) == UNQUOTE) {
        printf(",");
        if (tail(v) != EMPTY_LIST) {
            cf_print(head(tail(v)));
        }
        return;
    }

    if (head(v) == UNQUOTE_SPLICING) {
        printf(",@");
        if (tail(v) != EMPTY_LIST) {
            cf_print(head(tail(v)));
        }
        return;
    }

    printf("(");

    if (head(v) == RELOCATED_MARK) {
        printf("Relocated");
        cf_print(tail(v));
        return;
    }

    while (1) {
        cf_print(head(v));
        v = tail(v);
        if (v == EMPTY_LIST) {
            break;
        }
        printf(" ");
    }
    printf(")");
}

CF_API void cf_print(value_t v)
{
    switch (type_of(v)) {
    case TYPE_LIST:
        print_list(v);
        break;
    case TYPE_NUM:
        printf("%d", fl_num_val(v));
        break;
    case TYPE_SYM:
        if (v == UNBOUND) {
            printf("unbound");
        } else {
            printf("%s", sym_val(v)->name);
        }
        break;
    case TYPE_BUILTIN:
        printf("#<builtin %s >", builtin_names[builtin_val(v)->code]);
        break;
    default:
        printf("default");
        break;
    }
}

CF_API void cf_println(value_t v)
{
    cf_print(v);
    NL;
}

/*
 * pprint ─ 结构化打印（pretty-print）
 *
 * 与 print 不同，pprint 对嵌套列表进行缩进排版，使之更易读。
 * 打印规则：
 *   - 原子（数字/符号/builtin）直接打印
 *   - 空列表打印 ()
 *   - 含 quote/quasiquote/unquote 的列表使用简写 (', `, ,)
 *   - 元素全部为原子的列表打印在一行
 *   - 包含子列表的列表换行缩进（每层 2 空格）
 */

/* 判断列表是否包含嵌套的子列表 */
static int has_sublist(value_t v)
{
    while (v != EMPTY_LIST) {
        value_t h = head(v);
        if (is_list(h) && h != EMPTY_LIST
            && head(h) != QUOTE && head(h) != QUASIQUOTE
            && head(h) != UNQUOTE && head(h) != UNQUOTE_SPLICING) {
            return 1;
        }
        v = tail(v);
    }
    return 0;
}

static void pprint_(value_t v, int depth)
{
    switch (type_of(v)) {
    case TYPE_LIST:
        if (v == EMPTY_LIST) {
            printf("()");
            return;
        }
        if (head(v) == RELOCATED_MARK) {
            printf("<relocated>");
            return;
        }
        /* 简写形式 */
        if (head(v) == QUOTE && tail(v) != EMPTY_LIST) {
            printf("'");
            pprint_(head(tail(v)), depth);
            return;
        }
        if (head(v) == QUASIQUOTE && tail(v) != EMPTY_LIST) {
            printf("`");
            pprint_(head(tail(v)), depth);
            return;
        }
        if (head(v) == UNQUOTE && tail(v) != EMPTY_LIST) {
            printf(",");
            pprint_(head(tail(v)), depth);
            return;
        }
        if (head(v) == UNQUOTE_SPLICING && tail(v) != EMPTY_LIST) {
            printf(",@");
            pprint_(head(tail(v)), depth);
            return;
        }
        /* 决定单行还是多行 */
        if (has_sublist(v)) {
            printf("(");
            pprint_(head(v), depth + 1);
            v = tail(v);
            while (v != EMPTY_LIST) {
                printf("\n");
                for (int i = 0; i <= depth; i++) {
                    printf("  ");
                }
                pprint_(head(v), depth + 1);
                v = tail(v);
            }
            printf(")");
        } else {
            /* 单行打印 */
            printf("(");
            while (1) {
                pprint_(head(v), depth);
                v = tail(v);
                if (v == EMPTY_LIST) {
                    break;
                }
                printf(" ");
            }
            printf(")");
        }
        break;
    case TYPE_NUM:
        printf("%d", fl_num_val(v));
        break;
    case TYPE_SYM:
        if (v == UNBOUND) {
            printf("unbound");
        } else {
            printf("%s", sym_val(v)->name);
        }
        break;
    case TYPE_BUILTIN:
        printf("#<builtin %s >", builtin_names[builtin_val(v)->code]);
        break;
    default:
        printf("default");
        break;
    }
}

CF_API void cf_pprint(value_t v)
{
    pprint_(v, 0);
    NL;
}

static inline void indent(int depth)
{
    printf("%*s", depth, "");
}

static void smprint_inner(value_t v, int depth)
{
    indent(depth);
    type_t t = tag(v);
    switch (t) {
    case TAG_NUM:
        printf("[ NUM 0x%p] ", (void*)v);
        cf_println(v);
        break;
    case TAG_LIST:
        printf("[LIST 0x%p] ", (void*)v);
        cf_println(v);
        if (v != EMPTY_LIST && v != RELOCATED_MARK) {
            indent(depth + 2);
            printf("head:\n");
            smprint_inner(head_(v), depth + 4);
            indent(depth + 2);
            printf("tail:\n");
            smprint_inner(tail_(v), depth + 4);
        }
        break;
    case TAG_SYM:
    {
        Symbol* s = sym_val(v);
        printf("[ SYM 0x%p] name=\"%s\"  hash=%u  binding=0x%p\n",
            (void*)v, s->name, s->hash, (void*)s->binding);
        indent(depth + 2);
        printf("left=%p  right=%p\n", (void*)s->left, (void*)s->right);
        break;
    }
    case TAG_OTHER:
    {
        void* p = ptr(v);
        Type* tp = (Type*)p;
        if (tp->type == TYPE_BUILTIN) {
            Builtin* b = (Builtin*)p;
            printf("[BUILTIN 0x%p] code=%s\n", (void*)v, builtin_names[b->code]);
        } else {
            printf("[OTHER 0x%p] type=%zu\n", (void*)v, (size_t)tp->type);
        }
        break;
    }
    default:
        printf("[UNKNOWN 0x%p] tag=%zu\n", (void*)v, (size_t)t);
        break;
    }
}

CF_API void cf_smprint(value_t v)
{
    smprint_inner(v, 0);
}
