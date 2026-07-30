# cflisp 设计文档

## 设计目标

cflisp 的设计目标不是生产级 Lisp，而是**以最精简的代码实现一个完整的 Lisp 解释器**，作为学习编译器/解释器实现的教材。核心设计原则：

1. **极简** — 每个模块只做一件事，C 源码总量约 1500 行
2. **自洽** — 宏系统、准引用等高级特性尽可能用 Lisp 自身实现（system.lsp），而非 C
3. **教学友好** — 数据结构直观，命名清晰，标签/指针混用展示底层技巧


## 关键设计决策

### Tagged Pointer（标签指针）

所有值用 `uintptr_t`（`value_t`）表示，低 2 位作类型标签：

| Tag | 类型 | 值的含义 |
|-----|------|---------|
| 0x0 | NUM | 数值左移 2 位 |
| 0x1 | LIST | 堆指针 \| 0x1 |
| 0x2 | SYM | BST 节点指针 \| 0x2 |
| 0x3 | OTHER | 堆指针 \| 0x3（Builtin 等） |

**为何这么设计：**

- **零成本类型分派** — 无需额外的类型字段，`tag(x)` 一个 `&` 操作即得类型
- **数值直接内联** — 小整数不占用堆空间，避免 GC 压力
- **指针天然对齐** — 堆分配的对象至少 4 字节对齐，低 2 位恒为 0，可安全借用

**代价：** 仅支持 30 位整数（32 位系统）或 62 位整数（64 位系统），不支持浮点数（需扩展 tag 位或另用堆对象）。

### 符号表采用二叉搜索树（BST）

符号存在 C 堆（`malloc`），非 GC 管理。按 hash 值排序：

```c
typedef struct _Symbol {
    value_t binding;     // 符号绑定的值
    hash_t hash;         // 字符串 hash
    struct _Symbol* left;
    struct _Symbol* right;
    char name[1];        // 柔性数组
} Symbol;
```

**为何这么设计：**

- **全局唯一性** — 相同名字的符号复用同一 `Symbol` 节点
- **无 GC 干扰** — 符号表不受 GC 影响，GC 只追踪 `binding` 字段
- **柔性数组** — 一次 `malloc` 分配符号头和名字，减少碎片
- **BST 而非哈希表** — 实现简单，无需扩容 rehash；对小型符号表性能足够

### 环境栈布局

```
g_env_stack: [值, 符号, 值, 符号, ...]
                        ↑
                查找时从栈顶向下
```

**为何这么设计：**

- **一层数组两条目** — 每次 `env_push` 两次（先值后符号），查找时步长为 2
- **天然作用域嵌套** — 新闭包创建时 `push`，闭包求值后 `pop`
- **变量遮蔽** — 同名符号在栈中靠上的条目遮蔽靠下的

### Cheney 半区复制 GC

```
  ┌─────────────┐          ┌─────────────┐
  │  g_heap     │  ──→     │  g_newheap  │
  │  (from)     │  GC 后   │  (to)       │
  └─────────────┘          └─────────────┘
  (旧内容填 0x0A 毒化)      (存活对象复制至此)
```

**为何这么设计：**

- **实现极简** — 约 50 行 C 代码即完成
- **无碎片** — 复制后对象紧密排列，分配只需 bump-pointer
- **分配快** — `halloc` 只需移动指针 + 边界检查
- **教学价值** — 经典的 stop-and-copy 算法，便于理解 GC 原理

**代价：** 吞吐量减半（每次 GC 复制所有存活对象），不适合大堆。

### 深拷贝闭包捕获

```c
static value_t copy_body(value_t body)
```

创建闭包时，`fn`/`macro` 对函数体做深度复制，同时将环境栈中绑定的自由变量替换为实际值。

**为何这么设计：**

- **简化 GC** — 闭包和外部环境无共享引用，GC 无需处理环
- **语义简单** — 闭包创建后完全独立，无 shared-mutable 问题
- **实现直接** — 遍历函数体 AST，遇到符号就在环境栈中查找替换

**代价：** 大闭包创建开销高，不支持 set! 等赋值操作（因为修改闭包内的捕获值不会影响外部）。

### 尾递归优化（TCO）

通过 `tail_eval` 宏实现：

```c
#define tail_eval(exp) do { sexp = (exp); restore_stack(ss); goto eval_top; } while(0);
```

**为何这么设计：**

- **goto 而非 call** — 避免 C 栈增长，实现真正的尾递归
- **恢复栈指针** — 尾调用前释放当前栈帧（`restore_stack(ss)`），防止值栈泄漏
- **宏展开迭代** — `tail_macro` 计数器确保宏展开的尾位置自动展开


## 求值器设计

`eval_sexp` 是核心，用 `goto` 实现 TCO 和宏展开：

```
eval_top:
  type_of(sexp)
  ├── SYM → eval_sym (环境栈查找 → 符号表查找)
  └── LIST →
       ├── 求值 head 获得函数
       ├── 如果是 Builtin → apply_builtin (switch 分派)
       └── 如果是 List (fn/macro) → apply (准备环境 + 求值 body)
            └── 尾位置 → tail_eval (goto eval_top)
```

## system.lsp（自举式标准库）

**为何这么设计：**

系统库用 Lisp 自身编写，展现了"语言长大成人"的过程：

- `defmacro`、`defun`、`if`、`let` 用宏实现，避免在 C 中硬编码
- `quasiquote` 的展开算法（`qq-expand` / `qq-expand-list`）完全用 Lisp 实现，体现了 Lisp 的"代码即数据"
- `map`、`fold`、`append`、`zip` 等用纯 Lisp 实现，验证解释器的图灵完备性


## 代码组织原则

| 模块 | 职责 | 对外接口（暴露在 lisp.h 中） |
|------|------|------------------------|
| `lisp_main.c` | 入口 | `main()` |
| `lisp_core.c` | 初始化、GC、栈、类型判断 | `cf_lisp_init`, `cf_isNIL`, `type_of`, `push/pop` |
| `lisp_eval.c` | 求值器 | `cf_eval_toplevel`, `eval`, `cf_num_val` |
| `lisp_read.c` | 读取器 | `cf_read_file`, `read` |
| `lisp_print.c` | 打印机 | `cf_print`, `cf_println`, `cf_mprint` |
| `lisp_list.c` | 列表构造 | `cons`, `cons_`, `push_list`, `pop_list` |
| `lisp_symbol.c` | 符号表 | `symbol`, `dump_symtab` |
| `lisp_repl.c` | REPL | `cf_lisp_repl`, `fail` |

模块间依赖单向：`eval → read/print/list/symbol/core`, `main → all`。
