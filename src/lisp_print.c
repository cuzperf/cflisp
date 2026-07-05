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
        printf("%d", cf_num_val(v));
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
        if (v == UNBOUND) {
            printf("unbound\n");
        } else {
            Symbol* s = sym_val(v);
            printf("[ SYM 0x%p] name=\"%s\"  hash=%u  binding=0x%p\n",
                (void*)v, s->name, s->hash, (void*)s->binding);
            indent(depth + 2);
            printf("left=%p  right=%p\n", (void*)s->left, (void*)s->right);
        }
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
