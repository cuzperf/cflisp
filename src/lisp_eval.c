#include "lisp_internal.h"

/**
 * @brief 将 list 元素反向压栈
 * @note 没有将其放在 lisp_list.c 中，是因为仅在此文件中被使用，所以没必要暴露
 */
static void push_reverse_list(value_t l)
{
    if (l == EMPTY_LIST) {
        return;
    }
    push_reverse_list(tail(l));
    push(head(l));
}

static inline value_t to_bool(value_t v)
{
    return (v == NIL || v == EMPTY_LIST) ? NIL : T;
}

static inline number_t num_val(value_t x)
{
    assert_type(x, NUM);
    return (number_t)(x >> 2);
}

CF_API number_t cf_num_val(value_t x)
{
    return num_val(x);
}

/**
 * @brief 顶层求值，先列表参数反序压栈，再从最顶层往上求值
 */
CF_API value_t cf_eval_toplevel(value_t l)
{
    value_t res = NIL;
    cf_assert(g_sp == 0, "g_sp should be 0 when calling eval_toplevel");

    if (to_bool(l) == NIL) {
        return res;
    }

    assert_type(l , LIST);
    push_reverse_list(l);
    while (g_sp != 0) {
        value_t v = pop();
        res = eval(v);
    }
    return res;
}

static value_t eval_sexp(value_t sexp, bool noeval);

/**
 * @note 宏展开参数自然就不要先求值了
 */
static value_t expand(value_t v)
{
    return eval_sexp(v, true);
}

/**
 * @note 正常求值是参数先求值，再带入
 */
value_t eval(value_t v)
{
    return eval_sexp(v, false);
}

/**
 * @brief 尾递归优化
 */
#define tail_eval(exp) do { sexp = (exp); restore_stack(ss); goto eval_top; } while(0);

static void _assert_nargs(int _nargs, int n)
{
    cf_assert(_nargs == n, "Error: too %s arguments", (_nargs > n ? "many" : "few"));
}
#define assert_nargs(n) _assert_nargs(nargs, (n))

static value_t eqp(value_t v1, value_t v2);
static value_t copy_body(value_t body);
static void prepare_env(value_t args, int ss);
static value_t eval_sym(value_t v);

 /**
  * @brief 本文件中最核心的表达式求值函数
  * @param noeval 表示参数是否不要先求值
  */
static value_t eval_sexp(value_t sexp, bool noeval)
{
    value_t fun, funtype, args, body;
    BuiltinCode code;
    int nargs, sum;
    int ss = g_sp, ee = g_env_sp;

    int tail_macro = 1; // 宏展开深度计数器 - 每一轮减 1，避免宏无限展开

    value_t res = NIL;
    bool is_apply = false;  // 是否在 apply 流程中 - 控制 apply_top 而非 eval_top 进入
eval_top:
    if (tail_macro > 0) {
        --tail_macro;
    }
    switch (type_of(sexp)) {
    case TYPE_SYM:
        res = eval_sym(sexp);
        break;

    case TYPE_LIST:
        push(tail(sexp));       // 实参压栈（实参永远是一个列表）
        fun = eval(head(sexp)); // 获取函数
apply_top:
        if (type_of(fun) == TYPE_BUILTIN) {
            goto apply_builtin;
        }
        // NOTE: 不是内置函数就是 lambda 函数或宏 [陈智鹏@2026-7-11]
        if (type_of(fun) == TYPE_LIST) {
            args = (head(tail(fun)));   // 形参
            body = tail(tail(fun));     // 函数体
            goto apply;
        }
        error("Applying not a function");
        break;  // LCOV_EXCL_LINE
    default:
        res = sexp;
        break;
    }
    goto end;

apply_builtin:
    code = builtin_val(fun)->code;
    args = pop(); // 实参在前面已经压栈，这里出栈即可获取实参
    if (code >= F_ADD) {
        // 后面的都是普通函数，需要先对入参求值（apply_top 除外）
        push_reverse_list(args);
        if (!is_apply) {
            for (int i = g_sp - 1; i >= ss; --i) {
                g_stack[i] = eval(g_stack[i]);
            }
        }
        is_apply = false;
    }
    // NOTE: 入参被 push_reverse_list 后，栈涨了多少表示参数有多少 [陈智鹏@2026-7-11]
    nargs = g_sp - ss;

    // NOTE: 下面处理过程中入参被消耗（被 pop 出栈） [陈智鹏@2026-7-11]
    switch (code) {
    case F_ADD:
        cf_assert(nargs > 0, "Too few arguments");
        sum = num_val(pop());
        while (g_sp > ss) {
            sum += num_val(pop());
        }
        res = number(sum);
        break;
    case F_SUB:
        cf_assert(nargs > 0, "Too few arguments");
        sum = num_val(pop());
        if (nargs == 1) {
            sum = -sum; // 这里是负数特殊处理方式！
        }
        while (g_sp > ss) {
            sum -= num_val(pop());
        }
        res = number(sum);
        break;
    case F_MUL:
        cf_assert(nargs > 0, "Too few arguments");
        sum = num_val(pop());
        while (g_sp > ss) {
            sum *= num_val(pop());
        }
        res = number(sum);
        break;
    case F_DIV:
        cf_assert(nargs > 0, "Too few arguments");
        sum = num_val(pop());
        while (g_sp > ss) {
            number_t num = num_val(pop());
            cf_assert(num != 0, "Division by zero");
            sum /= num;
        }
        res = number(sum);
        break;
    case F_LT:
    {
        assert_nargs(2);
        number_t n1 = num_val(pop());
        number_t n2 = num_val(pop());
        if (n1 < n2) {
            res = T;
        } else {
            res = NIL;
        }
    }
    break;
    case F_GT:
    {
        assert_nargs(2);
        number_t n1 = num_val(pop());
        number_t n2 = num_val(pop());
        if (n1 > n2) {
            res = T;
        }
    }
    break;
    case F_EQ:
    {
        assert_nargs(2);
        value_t v1 = pop();
        value_t v2 = pop();
        res = eqp(v1, v2);
    }
    break;
    case F_NOT:
        assert_nargs(1);
        if (to_bool(pop()) == NIL) {
            res = T;
        } else {
            res = NIL;
        }
        break;
    case F_HEAD:
    {
        assert_nargs(1);
        value_t v = pop();
        assert_type(v, LIST);
        res = head(v);
    }
    break;
    case F_TAIL:
    {
        assert_nargs(1);
        assert_type(g_stack[ss], LIST);
        res = tail(g_stack[ss]);
    }
    break;
    case F_LISTP:
        assert_nargs(1);
        res = type_of(g_stack[ss]) == TYPE_LIST ? T : NIL;
        break;
    case F_SYMBOLP:
        assert_nargs(1);
        res = type_of(g_stack[ss]) == TYPE_SYM ? T : NIL;
        break;
    case F_NUMBERP:
        assert_nargs(1);
        res = type_of(g_stack[ss]) == TYPE_NUM ? T : NIL;
        break;
    case F_BUILTINP:
        assert_nargs(1);
        res = type_of(g_stack[ss]) == TYPE_BUILTIN ? T : NIL;
        break;
    case B_COND:
        push_list(args);
        for (int i = ss; i < g_sp; ++i) {
            value_t pair = g_stack[i];
            value_t cond = head(pair);
            push(head(tail_(pair)));
            res = eval(cond);
            value_t cur = pop();
            if (res != NIL) {
                if (tail_macro > 0) {
                    ++tail_macro;
                }
                tail_eval(cur);
                break;
            }
        }
        break;
    case B_DEF:
    {
        const char* name = sym_val(head(args))->name;
        value_t sym = symbol(name, &symtab);
        res = eval(head(tail_(args)));
        sym_val(sym)->binding = res;
    }
    break;
    case B_OR:
        push_reverse_list(args);
        while (g_sp > ss) {
            res = eval(pop());
            if (to_bool(res) != NIL) {
                break;
            }
        }
        break;
    case B_AND:
        push_reverse_list(args);
        while (g_sp > ss) {
            res = eval(pop());
            if (to_bool(res) == NIL) {
                break;
            }
        }
        break;
    case F_PRINT:
        while (g_sp > ss) {
            cf_println(pop());
        }
        break;
    case F_EVAL:
        assert_nargs(1);
        res = eval(g_stack[ss]);
        break;
    case F_APPLY:
    {
        fun = pop();
        is_apply = true;
        goto apply_top;
    }
    break;
    case B_MACRO:
    case B_FN:
    {
        int ee1 = g_env_sp;
        body = tail(args);
        args = head(args);
        push(args);
        // avoid unnessessary replacements while expanding macros
        if (!(code == B_FN && noeval)) {
            if (is_list(args)) {
                // push arg symbols to g_env_stack twice for shadowing
                for (value_t h = args; h != EMPTY_LIST; h = tail(h)) {
                    if (head(h) != REST) {
                        env_push(head(h));
                        env_push(head(h));
                    }
                }
            } else {
                env_push(args);
                env_push(args);
            }
        }
        body = copy_body(body);
        args = pop();
        res = cons_(code == B_FN ? FN : MACRO, cons_(args, body));
        env_restore_stack(ee1);
    }
    break;
    case B_DO:
        push_reverse_list(args);
        while (g_sp > ss) {
            res = eval(pop());
        }
        break;
    case F_CONS:
    {
        assert_nargs(2);
        value_t c1 = pop();
        value_t c2 = pop();
        res = cons(c1, c2);
        break;
    }
    case B_QUOTE:
        assert_nargs(0);
        res = head(args);
        break;
    default:                                // LCOV_EXCL_LINE
        error("Unknown builtin %d", code);  // LCOV_EXCL_LINE
    }
    goto end;
apply:
    {
        funtype = head(fun);
        cf_assert(funtype == FN || funtype == MACRO, "Applying not a function!!!!!");
        value_t list = pop();   // 获取实参列表
        push(body);
        push(args);

        int ss0 = g_sp;
        push_list(list);        // 可以确定实参个数了
        for (int i = ss0; i < g_sp; ++i) {
            if (funtype == FN && !is_apply) {
                g_stack[i] = eval(g_stack[i]);
            }
        }

        is_apply = false;
        args = g_stack[ss0 - 1];
        env_restore_stack(ee);

        prepare_env(args, ss0); // argnames
        args = pop();
        body = pop();

        push_reverse_list(body);

        while (g_sp > ss) {
            value_t cur = pop();
            if (g_sp == ss) {
                if (funtype == MACRO) {
                    tail_macro += 2;
                } else {
                    if (tail_macro) {
                        ++tail_macro;
                    }
                }
                tail_eval(cur);
            } else {
                res = eval(cur);
            }
        }
    }
end:                                // LCOV_EXCL_LINE
    if (tail_macro && !noeval) {
        tail_eval(res);
    }
    restore_stack(ss);
    env_restore_stack(ee);
    return res;
}

/**
 *
 * @brief 闭包创建时的变量捕获函数
 * 当解释器执行 fn 或 macro 时调用，它深拷贝函数体
 * 同时将当前环境中绑定的自由变量替换为实际值 —实现深拷贝捕获语义
 */
static value_t copy_body(value_t body)
{
    int ss = g_sp;
    //int sum, nargs;
    switch (type_of(body)) {
    case TYPE_LIST:
        if (body == EMPTY_LIST || head(body) == QUOTE) {
            return body;
        }
        // check if it if a macro
        if (is_sym(head(body))) {
            push(body);
            value_t tmp = eval(head(body));

            body = pop();

            if (is_list(tmp) && head(tmp) == MACRO) {
                body = expand(body);
                return copy_body(body);
            }
        }
        // optimisation for not doing unnessessary copies
        if (g_env_sp == 0) {
            return body;
        }

        push_list(body);
        for (int i = ss; i < g_sp; ++i) {
            g_stack[i] = copy_body(g_stack[i]);
        }
        return pop_list(ss);

    case TYPE_SYM:
        // replace symbol from its value from environment
        for (int i = g_env_sp - 1; i > 0; i -= 2) {
            if (g_env_stack[i] == body) {
                return g_env_stack[i - 1];
            }
        }
        return body;

    default:
        return body;
    }
}

static void prepare_env(value_t args, int ss)
{
    int sp = ss;
    if (is_list(args)) {
        for (value_t h = args; h != EMPTY_LIST; h = tail(h)) {
            if (head(h) != REST) {
                if (ss == g_sp) {
                    error("Not enough args");
                }
                env_push(g_stack[ss]);
                env_push(head(h));
                ++ss;
            } else {
                args = head(tail(h));
                break;
            }
        }
    }

    // NOTE: 注意在前面的判断中, args 可能被改变 [陈智鹏@2026-6-29]
    if (!is_list(args)) {
        value_t list = pop_list(ss);
        env_push(list);
        env_push(args);
    }

    restore_stack(sp);
}

/**
 * @note 环境栈 g_env_stack 的布局是 [值, 符号名, 值, 符号名, ...]，每对是 [value, symbol]。遇到符号时倒序查找：
 * 找到 → 返回符号当前绑定的值（替换为实际值，完成捕获）
 * 没找到 → 返回符号本身（自由变量，运行时在全局环境查找）
 */
static value_t eval_sym(value_t v)
{
#if 0
    if (v == UNBOUND) {
        error("Cannot eval unbound");
    }
#endif

    for (int i = g_env_sp - 1; i >= 0; i -= 2) {
        if (g_env_stack[i] == v) {
            return g_env_stack[i - 1];
        }
    }

    value_t sym = symbol(sym_val(v)->name, &symtab);
    UNUSED(sym);

    return sym_val(v)->binding;
}

/**
 * @brief 判断两个值是否一致（而非内存一致）
 */
static value_t eqp(value_t v1, value_t v2)
{
    if (type_of(v1) != TYPE_LIST || type_of(v2) != TYPE_LIST) {
        return v1 == v2 ? T : NIL;
    }

    while (v1 != EMPTY_LIST && v2 != EMPTY_LIST) {
        if (eqp(head(v1), head(v2)) == NIL) {
            return NIL;
        }
        v1 = tail(v1);
        v2 = tail(v2);
    }
    return v1 == v2 ? T : NIL;
}

// 未被使用
#if 0
static void prepare_args(value_t args)
{
    int ss = g_sp;
    push_list(args);

    for (int i = ss; i < g_sp; ++i) {
        g_stack[i] = eval(g_stack[i]);
    }
}
#endif
