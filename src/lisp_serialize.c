#include "lisp_internal.h"

#include <stdio.h>

/**
 * @brief 镜像（image）序列化 / 反序列化模块
 *
 * 解释器运行时的"功能状态"完全由全局符号表 symtab 刻画：每个 Symbol 的 name、
 * hash、左右子树指针，以及它的 binding（指向 GC 堆上的 List / 内置函数 / 符号 /
 * 数字）。因此只需把整棵符号表（连同每个 binding 指向的值图）序列化下来，即可在
 * 启动时完整还原 system.lsp 执行后的等价状态。
 *
 * 值的编码方式（每个值前导一个字节类型标记）：
 *   'Z'  EMPTY_LIST（空列表哨兵，tag == TAG_LIST 但无指针）
 *   'U'  UNBOUND（未绑定哨兵，tag == TAG_SYM 但无指针）
 *   'N'  NUM：紧跟 int32 数值
 *   'S'  SYM：紧跟以长度前缀的字符串（符号名，反序列化时按名查表得到同一节点）
 *   'B'  OTHER（此处仅内置函数）：紧跟 uint32 的 BuiltinCode
 *   'L'  LIST：先递归编码 head，再递归编码 tail
 *
 * 符号表按先序（pre-order）写出：每个节点 = '1' + 名字 + binding + 左子树 + 右子树；
 * 空子树用 '0' 标记。反序列化分两阶段：先重建整棵树（binding 留空），再按同样的
 * 先序遍历回填 binding（此时所有符号均已存在，按名查表必然命中，包括 NIL/T/REST
 * 这类自引用符号）。
 */

#define IMG_MAGIC "CFIMG"
#define IMG_MAGIC_LEN 5
#define IMG_VERSION 1

static FILE* g_img = NULL;
static bool g_img_err = false;

extern Builtin g_builtins[N_BUILTINS];

// ---------------------------------------------------------------------------
// 底层读写辅助
// ---------------------------------------------------------------------------

static void write_str(const char* s)
{
    uint32_t len = (uint32_t)strlen(s);
    if (fwrite(&len, sizeof(len), 1, g_img) != 1) {
        g_img_err = true;
    }
    if (fwrite(s, 1, len, g_img) != len) {
        g_img_err = true;
    }
}

static char* read_str()
{
    uint32_t len = 0;
    if (fread(&len, sizeof(len), 1, g_img) != 1) {
        g_img_err = true;
        return NULL;
    }
    char* s = malloc(len + 1);
    if (fread(s, 1, len, g_img) != len) {
        g_img_err = true;
        free(s);
        return NULL;
    }
    s[len] = '\0';
    return s;
}

// ---------------------------------------------------------------------------
// 值图序列化
// ---------------------------------------------------------------------------

static void write_value(value_t v)
{
    if (v == EMPTY_LIST) {
        fputc('Z', g_img);
        return;
    }
    if (v == UNBOUND) {
        fputc('U', g_img);
        return;
    }
    switch (tag(v)) {
    case TAG_NUM:
    {
        fputc('N', g_img);
        int32_t n = (int32_t)cf_num_val(v);
        if (fwrite(&n, sizeof(n), 1, g_img) != 1) {
            g_img_err = true;
        }
    }
    break;
    case TAG_SYM:
        fputc('S', g_img);
        write_str(sym_val(v)->name);
        break;
    case TAG_OTHER:
    {
        fputc('B', g_img);
        BuiltinCode code = builtin_val(v)->code;
        if (fwrite(&code, sizeof(code), 1, g_img) != 1) {
            g_img_err = true;
        }
    }
    break;
    case TAG_LIST:
        fputc('L', g_img);
        write_value(head(v));
        write_value(tail(v));
        break;
    default:
        g_img_err = true;
        break;
    }
}

static void save_symtab(Symbol* s)
{
    if (s == NULL) {
        fputc('0', g_img);
        return;
    }
    fputc('1', g_img);
    write_str(s->name);
    write_value(s->binding);   // 注意：binding 写在左右子树之前
    save_symtab(s->left);
    save_symtab(s->right);
}

// ---------------------------------------------------------------------------
// 值图反序列化
// ---------------------------------------------------------------------------

static value_t read_value()
{
    int t = fgetc(g_img);
    if (t == EOF) {
        g_img_err = true;
        return UNBOUND;
    }
    switch (t) {
    case 'Z':
        return EMPTY_LIST;
    case 'U':
        return UNBOUND;
    case 'N':
    {
        int32_t n = 0;
        if (fread(&n, sizeof(n), 1, g_img) != 1) {
            g_img_err = true;
            return UNBOUND;
        }
        return number((value_t)(intptr_t)n);
    }
    case 'S':
    {
        char* name = read_str();
        if (name == NULL) {
            return UNBOUND;
        }
        // NOTE: 整棵树在回填 binding 前已建好，按名查表必然命中 [陈智鹏@2026-7-17]
        value_t v = symbol(name, &symtab);
        free(name);
        return v;
    }
    case 'B':
    {
        BuiltinCode code = (BuiltinCode)0;
        if (fread(&code, sizeof(code), 1, g_img) != 1) {
            g_img_err = true;
            return UNBOUND;
        }
        return tagptr(&g_builtins[code], TAG_OTHER);
    }
    case 'L':
    {
        value_t h = read_value();
        value_t tl = read_value();
        return cons_(h, tl);
    }
    default:
        g_img_err = true;
        return UNBOUND;
    }
}

/**
 * @brief 第一阶段：重建整棵符号树（按先序），binding 暂置 UNBOUND
 * @note 节点的左右指针直接由镜像结构决定，与 hash 布局无关，保证查表一致
 */
static Symbol* load_build()
{
    int c = fgetc(g_img);
    if (c == '0') {
        return NULL;
    }
    if (c != '1') {
        g_img_err = true;
        return NULL;
    }
    char* name = read_str();
    if (name == NULL) {
        return NULL;
    }
    value_t vsym = symbol(name, &symtab);
    Symbol* s = sym_val(vsym);
    free(name);
    s->left = load_build();
    s->right = load_build();
    return s;
}

/**
 * @brief 第二阶段：按先序回填每个符号的 binding
 * @note 先序遍历顺序与 save_symtab 写入顺序严格对应，从而与文件中的 binding 对齐
 */
static void load_bindings(Symbol* s)
{
    if (s == NULL) {
        return;
    }
    s->binding = read_value();
    load_bindings(s->left);
    load_bindings(s->right);
}

// ---------------------------------------------------------------------------
// 对外接口
// ---------------------------------------------------------------------------

CF_API void cf_save_image(const char* path)
{
    g_img = fopen(path, "wb");
    if (g_img == NULL) {
        return;
    }
    g_img_err = false;
    if (fwrite(IMG_MAGIC, 1, IMG_MAGIC_LEN, g_img) != IMG_MAGIC_LEN) {
        g_img_err = true;
    }
    uint32_t ver = IMG_VERSION;
    if (fwrite(&ver, sizeof(ver), 1, g_img) != 1) {
        g_img_err = true;
    }
    save_symtab(symtab);
    fclose(g_img);
    g_img = NULL;
}

CF_API bool cf_load_image(const char* path)
{
    g_img = fopen(path, "rb");
    if (g_img == NULL) {
        return false;
    }
    g_img_err = false;

    char magic[IMG_MAGIC_LEN];
    if (fread(magic, 1, IMG_MAGIC_LEN, g_img) != IMG_MAGIC_LEN ||
        memcmp(magic, IMG_MAGIC, IMG_MAGIC_LEN) != 0) {
        fclose(g_img);
        g_img = NULL;
        return false;
    }
    uint32_t ver = 0;
    if (fread(&ver, sizeof(ver), 1, g_img) != 1 || ver != IMG_VERSION) {
        fclose(g_img);
        g_img = NULL;
        return false;
    }

    Symbol* saved = symtab;
    symtab = NULL;

    load_build();
    if (g_img_err || ferror(g_img)) {
        symtab = saved;
        fclose(g_img);
        g_img = NULL;
        return false;
    }

    load_bindings(symtab);
    if (g_img_err || ferror(g_img)) {
        symtab = saved;
        fclose(g_img);
        g_img = NULL;
        return false;
    }

    // NOTE: 重新指派 C 侧的全局符号指针，使其指向重建后的节点 [陈智鹏@2026-7-17]
    FN = symbol("fn", &symtab);
    MACRO = symbol("macro", &symtab);
    QUOTE = symbol("quote", &symtab);
    QUASIQUOTE = symbol("quasiquote", &symtab);
    UNQUOTE = symbol("unquote", &symtab);
    UNQUOTE_SPLICING = symbol("unquote-splicing", &symtab);
    NIL = symbol("nil", &symtab);
    T = symbol("#t", &symtab);
    REST = symbol("&", &symtab);

    // 自引用符号的绑定（镜像中已保存，这里再保险地确认一次）
    sym_val(NIL)->binding = NIL;
    sym_val(T)->binding = T;
    sym_val(REST)->binding = REST;

    fclose(g_img);
    g_img = NULL;
    return true;
}
