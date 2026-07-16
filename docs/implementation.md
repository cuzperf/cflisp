# cflisp 实现文档

本文档指导如何从头快速实现一个类似 cflisp 的 Lisp 解释器，按推荐实现顺序组织。

## 实现路线图

```
第1步：值表示 + 基础数据结构  →  tagged pointer + List + Symbol
第2步：读取器（Reader）       →  文本 → AST
第3步：打印器（Printer）      →  AST → 文本
第4步：求值器（Evaluator）    →  AST → 结果
第5步：内建函数               →  算术、列表操作、类型判断
第6步：特殊形式               →  quote/cond/def/fn/do/macro
第7步：环境与闭包             →  词法作用域 + 深拷贝捕获
第8步：尾递归优化             →  tail_eval 宏
第9步：垃圾回收               →  Cheney 复制 GC
第10步：REPL + 错误处理       →  交互循环 + setjmp/longjmp
第11步：标准库（system.lsp）  →  宏 + 常用函数
```

## 第1步：值表示

### Tagged Pointer 实现

```c
typedef uintptr_t value_t;

#define TAG_NUM   0x0
#define TAG_LIST  0x1
#define TAG_SYM   0x2
#define TAG_OTHER 0x3

#define tag(x)    ((x) & 0x3)
#define ptr(x)    ((void*)((x) & ~(value_t)0x3))
#define tagptr(x, t) (((value_t)(x)) | (t))
#define number(x) (((value_t)(x)) << 2)

#define is_num(x)  (tag(x) == TAG_NUM)
#define is_list(x) (tag(x) == TAG_LIST)
#define is_sym(x)  (tag(x) == TAG_SYM)
```

关键点：
- 数值：`(int_value << 2) | 0x0`，提取时算术右移 `(int)(x >> 2)`
- 指针：低 2 位用 OR 标签，使用时 AND 清除取指

### List 结构

```c
typedef struct { value_t head; value_t tail; } List;
```

cons 单元中 `tail` 必须为 `TAG_LIST` 类型（空列表 `EMPTY_LIST = (value_t)TAG_LIST`，即 `(void*)0x1`）。

### Symbol 结构（BST 符号表）

```c
typedef struct _Symbol {
    value_t binding;
    uint32_t hash;
    struct _Symbol* left;
    struct _Symbol* right;
    char name[1];       // 柔性数组
} Symbol;
```

- `binding` 存该符号绑定的值
- `hash` 用简单字符串哈希（如累加字符后异或常量）
- BST 按 hash 值排序，插入/查找迭代实现

### Builtin 结构

```c
typedef struct { type_t type; BuiltinCode code; } Builtin;
```

`type` 恒为 `TYPE_BUILTIN(=3)`，`code` 枚举标识具体内建函数。

## 第2步：读取器（Reader）

### 字符流处理

```
(text) ─→ read() ─→ g_stack (value_t 栈)
```

核心逻辑：

```c
void read(FILE* f, Symbol** env) {
    char c = fpeekc(f);
    switch (c) {
        case '(':  read_list(f, env);  break;       // 列表
        case '\'': read(f, env);                    // 引号糖
                  push(make_list(QUOTE, pop(), END));
                  break;
        case '`':  ...  // quasiquote 糖
        case ',':  ...  // unquote / unquote-splicing 糖
        case '0'..'9': read_int(f, env); break;     // 整数
        case ';':  skip_comments(f); goto start;    // 注释
        default:   read_sym(f, env); break;         // 符号
    }
}
```

技巧：`fpeekc` 用 `getc + ungetc` 实现 peek，`skip_spaces` 跳过空白。

> 核心：read 函数总会向值栈 push 一个元素，如果不保证会使得实现变得很复杂

### 列表读取

```
read_list:
  '(' → 递归 read 各元素 → 遇到 ')' 结束
       → pop_list(ss) 将栈上元素打包成链表
```

### 多表达式文件

`cf_read_file` 反复调用 `read` 直至 EOF，每次结果用 `make_cell` 包裹后用 `tail` 串联成链表，最后 `cf_eval_toplevel` 依次求值。


## 第3步：打印器（Printer）

### 类型分派

```c
void cf_print(value_t v) {
    switch (type_of(v)) {
        case TYPE_NUM:    printf("%d", num_val(v)); break;
        case TYPE_LIST:   print_list(v); break;
        case TYPE_SYM:    printf("%s", sym_val(v)->name); break;
        case TYPE_BUILTIN: printf("#<builtin %s>", builtin_names[b->code]); break;
    }
}
```

### 列表打印

递归打印 head，迭代遍历 tail，遇到 `QUOTE`/`QUASIQUOTE`/`UNQUOTE`/`UNQUOTE_SPLICING` 时输出对应糖语法前缀。

## 第4步：求值器（Evaluator）

### 顶层求值

```c
value_t cf_eval_toplevel(value_t l) {
    push_reverse_list(l);   // 反序压栈
    while (g_sp != 0) {
        value_t v = pop();
        res = eval(v);      // 依次求值
    }
    return res;             // 返回最后一个
}
```

### eval 主循环

```c
eval_top:
  type_of(sexp)
    ├─ SYM  → 环境栈查找 → 符号表查找 → 返回 binding
    └─ LIST →
        push(tail(sexp))        // 存参数
        fun = eval(head(sexp))  // 求值函数位置
        if fun is BUILTIN → apply_builtin
        if fun is LIST (fn/macro) → apply
```

### 参数求值 vs 不求值

内建算术/比较类（`code >= F_ADD`）在 `apply_builtin` 中自动求值参数；特殊形式（`B_COND/B_DEF/B_QUOTE`）自行控制求值时机，不做预求值。

## 第5步：内建函数实现

用 `CF_BUILTIN_FUNCTIONS` 宏批量定义枚举和名字：

```c
#define CF_BUILTIN_FUNCTIONS(XX) \
    XX(B_FN,    "fn")     \
    XX(F_ADD,   "+")      \
    XX(F_CONS,  "cons")   \
    // ...

typedef enum {
    #define XX(s, n) s,
    CF_BUILTIN_FUNCTIONS(XX)
    #undef XX
} BuiltinCode;
```

初始化时创建全局 `Builtin` 数组，将每个内建函数名字注册到符号表，`binding` 指向对应 `Builtin` 结构。

### 典型实现：加法

```c
case F_ADD:
    sum = num_val(pop());
    while (g_sp > ss) sum += num_val(pop());
    res = number(sum);
    break;
```

### 比较 = 递归列表相等

`eqp` 先判类型：非列表直接 `==` 比较；列表递归比较 `head` 和 `tail`。


## 第6步：环境与作用域

### 环境栈

```
g_env_stack: [value1, sym1, value2, sym2, ...]
```

- `fn`/`macro` 创建时：形参符号和当前值入栈配对
- `eval_sym` 符号查找：从栈顶步长 2 向下遍历，找到即返回值
- 未找到 → 到符号表取全局 `binding`

### 深拷贝捕获（闭包）

```c
static value_t copy_body(value_t body) {
    case TYPE_LIST:  递归复制每个元素
    case TYPE_SYM:   在环境栈中查找替换为实际值
    default:         返回自身（数值等）
}
```

创建闭包时（B_FN/B_MACRO）调用 `copy_body`，将函数体中的自由变量替换为当前值。


## 第7步：尾递归优化

```c
#define tail_eval(exp) do { \
    sexp = (exp);           \
    restore_stack(ss);      \
    goto eval_top;          \
} while(0);
```

用法：函数体最后一个表达式求值前，检测到是尾位置 → 用 `tail_eval` 而非 `eval`，避免值栈增长。


## 第8步：垃圾回收

### Cheney 半区复制

```
GC 触发：halloc 分配时 g_curheap + size > g_lim
GC 过程：
  1. 堆大小 *= 1.5（或 realloc g_newheap）
  2. 交换 g_heap ↔ g_newheap（from-space ↔ to-space）
  3. relocate 根集（g_stack, g_env_stack, symtab->binding）
  4. 旧半区 memset 0x0A 毒化
```

### relocate 实现

```c
static value_t relocate_list(value_t l) {
    if (l == EMPTY_LIST) return l;
    if (head(l) == RELOCATED_MARK) return tail(l); // 已搬迁
    tail(l) = _relocate_list(l);   // 复制到 to-space
    head(l) = RELOCATED_MARK;      // 标记已搬迁，tail 指向新位置
    return tail(l);
}
```

**关键技巧**：`RELOCATED_MARK` 作为 forwarding pointer 标记，head 存标记、tail 存新地址。

### 根集

| 根 | 内容 |
|----|------|
| `g_stack[0..g_sp)` | 值栈中的所有 value_t |
| `g_env_stack[0..g_env_sp)` | 环境栈中的 value_t（跳过符号条目） |
| `symtab` | 每个 Symbol 的 binding 字段 |


## 第9步：REPL

### 核心循环

```c
void cf_lisp_repl() {
    while (true) {
        printf(">");
        save_sp_ep = g_sp, g_env_sp;
        if (!setjmp(jmp_mark)) {
            read(stdin, &symtab);
            if (g_sp > save_sp) {
                res = eval(pop());
                cf_print(res);
            }
        } else {
            restore_stack(save_sp);  // 错误恢复
            env_restore_stack(save_ep);
        }
    }
}
```

### fail() 错误处理

```c
void fail() {
    if (in_repl) longjmp(jmp_mark, -1);
    else exit(1);
}
```

`error` 宏用 `fprintf + fail()`，可在 REPL 中安全恢复。


## 第10步：标准库（system.lsp）

### 实现策略

**用 Lisp 实现 Lisp：**

- `defmacro` 和 `defun` 是宏，展开为 `def` + `macro`/`fn`
- `if` 宏展开为 `cond`
- `let` 宏展开为 `((fn (vars) body) vals)`
- `quasiquote` 展开算法完全在 Lisp 层实现
- `map`/`fold`/`append`/`zip` 等用纯递归实现

### 关键宏：splice-body

```lisp
(def splice-body
  (fn (body)
    (cond ((atom? body) body)
          ((= (tail body) '()) (head body))
          (else (cons do body)))))
```

功能：将多表达式函数体自动包裹 `do`，单表达式保持原样，`atom?` 兜底。

## 构建系统

### CMake 配置要点

```cmake
# 共享库（隐藏内部实现）
add_library(cflisplib SHARED src/lisp_core.c src/lisp_eval.c ...)
target_compile_definitions(cflisplib PRIVATE BUILDING_CF_LIB)

# 可执行文件
add_executable(cflisp src/lisp_main.c system.lsp)

# 测试（Google Test）
add_subdirectory(third_party/googletest)
add_executable(unit_test unit_test/test_cflisp.cpp)
target_link_libraries(unit_test test_utils gtest cflisplib)
```

### 测试技巧

`cl_eval_string` 将 Lisp 代码字符串写入临时文件，然后 `cf_read_file` + `cf_eval_toplevel` 求值，最后删除临时文件。这是在没有 `read_string` 时的变通方案。

## 快速实现检查表

| 步骤 | 产出 | 验证方式 |
|------|------|---------|
| 值表示 | `value_t` 操作宏 | 测试 tag/untag 数值、构造 List |
| Reader | 解析 `(+ 1 2)` → 列表 | `cf_mprint` 看 AST |
| Printer | 打印任意 value_t | `cf_print(42)` → `42` |
| 数值运算 | `+ - * /` | `(+ 1 2)` → `3` |
| 符号 + def | `(def x 10)` + `x` | `x` → `10` |
| quote/cond | `(quote (1 2))`、条件分支 | 不求值引用、条件判断 |
| fn 闭包 | `((fn (x) (+ x 1)) 2)` | → `3` |
| 宏 | `macro` 创建 + 展开 | 宏展开结果 |
| GC | 大量分配触发 gc | `(head (range 2000))` 通过 |
| REPL | 交互式循环 + 错误恢复 | 输入错误不崩溃 |
| system.lsp | 所有标准库函数 | `example.lsp` 全部通过 |
