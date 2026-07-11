#include "lisp_internal.h"

#define INITIAL_HEAP_SIZE (64000)
#define STACK_SIZE (160 * 1024)
#define ENV_SIZE (160 * 1024)
#define HEAP_RESIZE_RATIO (1.5)

Builtin g_builtins[N_BUILTINS];

const char* builtin_names[N_BUILTINS + 1] = {
#define XX(symbol, name) name,
    CF_BUILTIN_FUNCTIONS(XX)
#undef XX
};

value_t FN, MACRO;
value_t QUOTE, UNQUOTE, QUASIQUOTE, UNQUOTE_SPLICING;
value_t REST;
value_t NIL, T;

typedef char* memory_t;
static int g_heap_size;
static memory_t g_heap, g_curheap, g_lim;
static memory_t g_oldheap = NULL;

Symbol* symtab = NULL;

static int g_stack_size = STACK_SIZE;
static int g_env_stack_size = ENV_SIZE;

CF_API void cf_lisp_init()
{
    for (int i = 0; i < N_BUILTINS; ++i) {
        g_builtins[i].type = TYPE_BUILTIN;
        g_builtins[i].code = (BuiltinCode)i;
        value_t tmp = symbol(builtin_names[i], &symtab);
        sym_val(tmp)->binding = tagptr(g_builtins + i, TAG_OTHER);
    }

    g_curheap = g_heap = malloc(INITIAL_HEAP_SIZE);
    g_lim = g_heap + INITIAL_HEAP_SIZE;
    g_heap_size = INITIAL_HEAP_SIZE;

    g_stack = malloc(g_stack_size * sizeof(value_t));
    g_env_stack = malloc(g_env_stack_size * sizeof(value_t));

    FN = symbol("fn", &symtab);
    MACRO = symbol("macro", &symtab);
    QUOTE = symbol("quote", &symtab);
    QUASIQUOTE = symbol("quasiquote", &symtab);
    UNQUOTE = symbol("unquote", &symtab);
    UNQUOTE_SPLICING = symbol("unquote-splicing", &symtab);

    NIL = symbol("nil", &symtab);
    sym_val(NIL)->binding = NIL;

    T = symbol("#t", &symtab);
    sym_val(T)->binding = T;

    REST = symbol("&", &symtab);
    sym_val(REST)->binding = REST;
}

CF_API bool cf_isNIL(value_t v)
{
    return v == EMPTY_LIST || v == NIL;
}

type_t type_of(value_t v)
{
    type_t t = tag(v);
    if (t < TAG_OTHER) {
        return t;
    }
    void* p = ptr(v);           // TODO rise error on null
    return ((Type*)p)->type;
}

// 栈
value_t* g_stack;
int g_sp = 0;

void push(value_t v)
{
    if (g_sp >= g_stack_size) {
        error("Stack overflow");
    }
    g_stack[g_sp++] = v;
}

value_t pop()
{
    return g_stack[--g_sp];
}

void restore_stack(int n)
{
    g_sp = n;
}

// 环境栈
value_t* g_env_stack;
int g_env_sp = 0;

void env_push(value_t v)
{
    if (g_env_sp >= g_env_stack_size) {
        error("Env overflow");
    }
    g_env_stack[g_env_sp++] = v;
}

void env_restore_stack(int n)
{
    g_env_sp = n;
}

// 堆和 gc
static void* halloc(size_t);
value_t make_cell(value_t v)
{
    push(v);
    List* cell = halloc(sizeof(List));
    cell->head = pop();
    cell->tail = EMPTY_LIST;
    return tagptr(cell, TAG_LIST);
}

static value_t relocate(value_t);
static void relocate_symtab(Symbol*);

bool is_gc = 0;
void gc()
{
    if (is_gc) {
        // NOTE: 如果 HEAP_RESIZE_RATIO 是 1.5 感觉这种可能性还是有的哦 [陈智鹏@2026-7-11]
        error("Gc in gc!!!!");
    }
    is_gc = true;

    int oh = g_heap_size;
    g_heap_size = (int)(g_heap_size * HEAP_RESIZE_RATIO);
    if (g_oldheap == NULL) {
        g_oldheap = malloc(g_heap_size);
    } else {
        g_oldheap = realloc(g_oldheap, g_heap_size);
    }

    memory_t t = g_heap;
    g_heap = g_oldheap;
    g_oldheap = t;

    g_curheap = g_heap;
    g_lim = g_heap + g_heap_size;

    //dump_stack();
    for (int ss = 0; ss < g_sp; ++ss) {
        g_stack[ss] = relocate(g_stack[ss]);
    }

    // dump_env();
    for (int ee = g_env_sp - 2; ee >= 0; ee -= 2) {
        // NOTE: SYM 是全局的，其本身无需 relocate [陈智鹏@2026-7-11]
        assert_type(g_env_stack[ee + 1], SYM);
        if (type_of(g_env_stack[ee]) != TYPE_SYM) {
            g_env_stack[ee] = relocate(g_env_stack[ee]);
        }
    }

    // NOTE: SYM 虽然无需 relocate，但其绑定的元素需要 relocate [陈智鹏@2026-7-11]
    relocate_symtab(symtab);

    is_gc = false;
    // g_heap poisoning for trapping bugs
    memset(g_oldheap, 0x0A, oh);
}

static void relocate_symtab(Symbol* sym)
{
    sym->binding = relocate(sym->binding);
    if (sym->left) {
        relocate_symtab(sym->left);
    }
    if (sym->right) {
        relocate_symtab(sym->right);
    }
}

static value_t relocate_list(value_t);

/**
 * @biref 就旧堆申请了内存的栈（包括环境栈）上元素重新定位到新堆上
 * @note 目前只有栈上只有 List 元素会在堆上申请内存
 */
static value_t relocate(value_t v)
{
    switch (type_of(v)) {
    case TYPE_LIST:
        return relocate_list(v);
    default:
        return v;
    }
}

/**
 * @brief relocate_list 就是把 head 和 tail 都处理一遍
 * @note tail(list) 必然也是 list
 */
static value_t _relocate_list(value_t l)
{
    value_t v = head(l);
    value_t t = tail(l);
    value_t cell = make_cell(relocate(v));
    if (t != EMPTY_LIST) {
        tail(cell) = relocate_list(t);
    }
    return cell;
}

/**
 * @brief 接口和防止重复 relocate 的函数
 * @note 处理后改变 l 的 head，返回 l 的 tail
 */
static value_t relocate_list(value_t l)
{
    if (l == EMPTY_LIST) {
        return l;
    }
    // NOTE: RELOCATED_MARK 表示已经被 relocate 过了就不处理了 [陈智鹏@2026-7-11]
    if (head(l) != RELOCATED_MARK) {
        tail(l) = _relocate_list(l);
        head(l) = RELOCATED_MARK;
    }
    return tail(l);
}

/**
 * @brief 堆上申请内存，堆满则 gc
 */
static void* halloc(size_t s)
{
    if (g_curheap + s >= g_lim) {
        gc();
    }
    memory_t h = g_curheap;
    g_curheap += s;
    return (void*)h;
}

// LCOV_EXCL_START

void dump_heap()
{
    printf("dump_heap begin---------------------------:\n");
    for (memory_t m = g_heap; m < g_curheap; ++m) {
        value_t val = ((List*)m)->head;
        cf_print(val);
        printf(" -|- ");
    }
    printf("dump_heap end---------------------------\n");
}

void dump_stack()
{
    printf("dump_stack begin---------------------------:\n");
    for (int i = 0; i < g_env_sp; ++i) {
        cf_println(g_env_stack[i]);
    }
    printf("dump_stack end---------------------------\n");
}

void dump_env()
{
    printf("dump_env begin---------------------------:\n");
    for (int i = 0; i < g_env_sp; i += 2) {
        cf_print(g_env_stack[i]);
        printf("\t");
        cf_println(g_env_stack[i + 1]);
    }
    printf("dump_env end---------------------------\n");
}

// LCOV_EXCL_STOP

// 未被使用的函数
#if 0

value_t top()
{
    return g_stack[g_sp - 1];
}

value_t popn(int n)
{
    g_sp -= n;
    return g_stack[g_sp];
}

value_t env_top()
{
    return g_env_stack[g_env_sp - 1];
}

value_t env_pop()
{
    return g_env_stack[--g_env_sp];
}
#endif
