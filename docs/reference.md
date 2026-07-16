# cflisp 内置功能参考手册

本手册为 cflisp 内部实现的文档（尽可能简短），主要包括

1. 数据类型表示
2. 内置符号和语法糖
3. 内置函数和宏
4. 内置库函数和宏

## 数据类型表示

所有值通过 `uintptr_t`（`value_t`）表示，低 2 位用作类型标签：

### 数值 (Number) 0x0

目前数值类型仅支持 32 位 int，且最后两位用于标记位，所以值域范围为 $[-2^{29}, 2^{29} - 1]$

```
42      → 42
-5      → (- 5)     ; 使用取负，-5 是符号
```

### 列表 (List) 0x1

由 cons 单元构成的链表。空列表写作 `'()`。

列表内存布局：
```
(a (b c) d)
  ↓
[a|●]──→[●|●]──→[d|()]
           ↓
          [b|●]──→[c|()]
```

### 符号 (Symbol) 0x2

用作变量名和字面量标识符。符号表以二叉搜索树（BST）组织，按 hash 值排序。

```
x          → 符号 x
'hello     → 符号 hello
def        → 符号 def（内建关键字）
```

特殊符号：
- `nil` — 假值，同时是空值
- `#t` — 真值
- `&` — 可变参数标记（rest 参数）

### 内建函数 (Builtin) 0x3

```
+           → #<builtin + >
cons        → #<builtin cons >
```

## 内置函数和宏

**特殊形式（8个）**：参数不自动求值

- `fn` — 创建函数（λ 表达式）
- `macro` — 创建宏
- `quote` / `'` — 阻止求值
- `def` — 定义/更新全局变量
- `cond` — 条件分支
- `do` — 顺序执行，返回最后一个值
- `and` — 短路与
- `or` — 短路或

**普通函数（18个）**：参数自动求值

- 算术：`+` `-` `*` `/`
- 比较：`<` `>` `=`
- 逻辑：`not`
- 列表：`cons` `head` `tail`
- 元编程：`eval` `apply`
- 类型谓词：`number?` `symbol?` `list?` `builtin?`
- 输出：`print`

### `fn` — 创建函数（λ 表达式）

`(fn (params) body)` 创建一个匿名函数。

```lisp
; 一元函数
(def double (fn (x) (* x 2)))
(print (double 5))                      ; 输出: 10

; 多元函数
(def add (fn (a b) (+ a b)))
(print (add 3 4))                       ; 输出: 7

; 任意参数（特殊情况）
(def list (fn args args))
(print (list 1 2 3))                    ; 输出 (1 2 3)

; 匿名函数直接调用
(print ((fn (x) (* x x)) 6))            ; 输出: 36
```

### `macro` — 创建宏

`(macro (params) body)` 创建一个宏。宏的参数**不**求值，宏体返回一个 S-表达式，该表达式再被求值。

```lisp
; 定义一个简单宏：将表达式执行两次
(def twice (macro (expr)
  (list 'do expr expr)))

(twice (print 42))                 ; 输出: 42  （打印两次）
                                   ;       42

; 宏的参数是不求值的原始表达式
(def my-if (macro (cond then else)
  (list 'cond (list cond then) (list 'else else))))

(print (my-if (< 1 2) 'yes 'no))  ; 输出: yes
```

### `quote` — 阻止求值

`(quote expr)` 返回 `expr` 本身，不对其求值。

```lisp
(print (quote a))      ; 输出: a （符号 a，不会去查找 a 变量的值）
(print (quote (1 2)))  ; 输出: (1 2) （列表，不会尝试调用 1 作为函数）
```

**语法糖**：`'expr` 等价于 `(quote expr)`，详见 `read` 函数

```lisp
(print 'hello)         ; 输出: hello
(print '(a b c))       ; 输出: (a b c)
(print '())            ; 输出: ()
```

### `def` — 定义/更新变量

`(def name value)` 定义全局变量（函数也是变量）。如果变量已存在，则更新其值。

```lisp
(def x 10)                         ; 定义 x = 10
(print x)                          ; 输出: 10

(def x (+ x 1))                    ; 可重新赋值
(print x)                          ; 输出: 11
```

### `cond` — 条件分支

`(cond (test1 expr1) (test2 expr2) ... (else exprN))` 依次求值各条件，第一个为真的条件对应的表达式被求值并返回。`else` 是兜底分支（其值为 `#t`，永远为真）。

```lisp
(print (cond ((< 1 2) 'yes) (else 'no)))                ; 输出: yes
(print (cond (nil 'case1) ('#t 'case2) (else 'case3)))  ; 输出: case2
(print (cond (nil 'first) (else 'last)))                ; 输出: last
```

> 条件判断规则：只有 `nil` 和 `'()` 为假，其他值（包括 `0`）均为真。

### `do` — 顺序执行

`(do expr1 expr2 ... exprN)` 依次求值所有表达式，返回最后一个表达式的值。

```lisp
(print (do 1 2 3))                 ; 输出: 3 （只返回最后一个）
(do (print 'step1) (print 'step2)) ; 输出: step1 （先执行第一个）
                                   ;      step2 （再执行第二个）
```

### `and` — 逻辑与（短路求值）

`(and expr1 expr2 ... exprN)` 从左到右求值，遇到假值（`nil` 或 `'()`）就停止并返回该假值；否则返回最后一个表达式的值。

```lisp
(print (and 1 2 3))                ; 输出: 3 （全部为真，返回最后一个）
(print (and 1 nil 3))              ; 输出: nil （遇到 nil 短路）
(print (and 0 1 2))                ; 输出: 2 （0 不是 nil，不会短路）
```

### `or` — 逻辑或（短路求值）

`(or expr1 expr2 ... exprN)` 从左到右求值，遇到真值就停止并返回该值；否则返回最后一个（假）值。

```lisp
(print (or nil 2 3))    ; 输出: 2 （遇到 2 短路）
(print (or nil nil))    ; 输出: nil （全假）
(print (or 0 1))        ; 输出: 0 （0 视为真，短路返回 0）
```

## 四则运算: `+`, `-`, `*`, `/`（多参数）

lisp 中四则运算可以接受多参数！

```lisp
(print (+ 1 2))         ; 输出: 3
(print (+ 1 2 3 4 5))   ; 输出: 15

(print (- 5))           ; 输出: -5  （单个参数时取负数）
(print (- 10 3))        ; 输出: 7   （10 - 3 = 7）
(print (- 10 3 2))      ; 输出: 5   （10 - 3 - 2 = 5）

(print (* 2))           ; 输出: 2
(print (* 2 3 4))       ; 输出: 24

(print (/ 12 2 3))      ; 输出: 2
(print (/ 10))          ; 输出: 10
(print (/ 10 3))        ; 输出: 3   （整数除法，向下取整）
(/ 1 0)                 ; 错误: Division by zero
```

> 注意 lisp 不接受 `-5`, 只能使用 `(- 5)`

## 比较运算符: `<`, `>`（两参数，仅接受数值标量）

```lisp
(print (< 2 3))   ; 输出: #t
(print (> 2 3))   ; 输出: nil
```

### `=` — 等于（值相等）

对数字按值比较，对符号按 identity 比较，对列表按元素递归比较。

```lisp
(print (= 3 4))                       ; 输出: nil
(print (= 'a 'a))                     ; 输出: #t  （符号相等）
(print (= '(1 2) '(1 3)))             ; 输出: nil
(print (= '(1 (2 3) 4) '(1 (2 3) 4))) ; 输出: #t  （列表元素递归相等）
```

## 逻辑函数: `not`

```lisp
(print (not nil))                  ; 输出: #t
(print (not '#t))                  ; 输出: nil
(print (not 0))                    ; 输出: nil （0 不是 nil，视为真）
(print (not '()))                  ; 输出: #t
```

### `cons` — 构造列表

`(cons head tail)` 构造一个 `(head . tail)` 的序对。`tail` **必须**是一个列表。

```lisp
(print (cons 1 '(2 3)))   ; 输出: (1 2 3)
(print (cons 1 '()))      ; 输出: (1)
```

### `head` — 取列表第一个元素（car）

```lisp
(print (head '(a b c)))         ; 输出: a
(print (head (cons 1 '(2 3))))  ; 输出: 1
(head '())                      ; 错误: Trying to take head/tail of empty list
```

### `tail` — 取列表剩余部分（cdr）

```lisp
(print (tail '(a b c)))   ; 输出: (b c)
(print (tail '(a)))       ; 输出: ()
; (tail '())              ; 错误: Trying to take head/tail of empty list
```

### `eval` — 显式求值（元编程函数）

`(eval expr)` 对表达式进行求值。

```lisp
(print (eval '(+ 1 2)))            ; 输出: 3
(print (eval (list '+ 1 2)))       ; 输出: 3 （动态构造表达式再求值）

; 与 def 结合实现动态变量访问
(def myvar 42)
(print (eval 'myvar))              ; 输出: 42
```

### `apply` — 以列表形式传递参数（元编程函数）

`(apply fn arglist)` 以列表中的元素作为参数调用函数。

```lisp
(print (apply + '(10 20 30)))      ; 输出: 60
(print (apply * '(2 3 4)))         ; 输出: 24
(print (apply (fn (x y) (+ x y)) '(10 20)))  ; 输出: 30
```

### 类型谓词

1. `number?`  - 是否为数字
2. `symbol?`  - 是否为符号
3. `list?`    - 是否为列表
4. `builtin?` - 是否为内置函数

```lisp
(print (number? 42))               ; 输出: #t
(print (number? 'x))               ; 输出: nil
(print (number? '(1 2)))           ; 输出: nil

(print (symbol? 'hello))           ; 输出: #t
(print (symbol? 42))               ; 输出: nil
(print (symbol? '#t))              ; 输出: #t

(print (list? '(1 2)))             ; 输出: #t
(print (list? 42))                 ; 输出: nil
(print (list? '()))                ; 输出: #t （空列表也是列表）

(print (builtin? +))               ; 输出: #t
(print (builtin? 42))              ; 输出: nil
(print (builtin? cons))            ; 输出: #t
```

### `print` — 输出

`(print value1 value2 ...)` 打印值到标准输出，每个值占一行，返回 `nil`。

```lisp
(print 42)              ; 输出: 42
(print '(a b c))        ; 输出: (a b c)
(print +)               ; 输出: #<builtin + >
(print 'hello `world)   ; 输出: hello
                        ;       world
(print '())             ; 输出: ()
```

### 内置函数速查表

| 名称 | 类型 | 参数 | 说明 |
|------|------|------|------|
| `fn` | 特殊 | 变参 | 创建函数 |
| `macro` | 特殊 | 变参 | 创建宏 |
| `quote` | 特殊 | 1 | 返回参数本身，不求值 |
| `def` | 特殊 | 2 | 定义/更新全局变量 |
| `cond` | 特殊 | 变参 | 条件分支 |
| `do` | 特殊 | 变参 | 顺序执行，返回最后一个值 |
| `and` | 特殊运算符 | 变参 | 短路与 |
| `or` | 特殊运算符 | 变参 | 短路或 |
| `+` | 运算符 | ≥1 | 加法 |
| `-` | 运算符 | ≥1 | 减法/取负 |
| `*` | 运算符 | ≥1 | 乘法 |
| `/` | 运算符 | ≥1 | 整数除法 |
| `<` | 运算符 | 2 | 小于比较 |
| `>` | 运算符 | 2 | 大于比较 |
| `=` | 运算符 | 2 | 相等比较 |
| `not` | 逻辑函数 | 1 | 逻辑非 |
| `cons` | 列表操作 | 2 | 构造列表 |
| `head` | 列表操作 | 1 | 取首元素 |
| `tail` | 列表操作 | 1 | 取剩余列表 |
| `eval` | 元编程 | 1 | 显式求值 |
| `apply` | 元编程 | 2 | 以列表参数调用 |
| `list?` | 谓语 | 1 | 列表判断 |
| `symbol?` | 谓语 | 1 | 符号判断 |
| `number?` | 谓语 | 1 | 数字判断 |
| `builtin?` | 谓语 | 1 | 内置函数判断 |
| `print` | 打印 | 变参 | 打印输出 |

## 内置符号和语法糖

| 名称 | 字符串 |说明 | 备注 |
|------|------|------|------|
| MACRO | `macro` |  内置函数对应符号 | |
| FN | `fn`  | 同上 | |
| QUOTE | `quote` | 同上 | 语法糖：`'` |
| QUASIQUOTE | `quasiquote` | 拟引用 | 语法糖：`` ` `` |
| UNQUOTE | `unquote` | 解引用 | 语法糖：`,` |
| UNQUOTE_SPLICING | `unquote-splicing` | 解引用分裂 | 语法糖：`,@`|
| NIL | `nil` | 布尔假值 | |
| T | `#t` |  布尔真值 | 空列表 `'()` 也是假值 |
| REST | `&` | 捕获剩余参数 | 重要的语言特征 |

内置符号在 `cf_lisp_init` 中定义。以下为额外解释：

1. 函数也是符号吗？函数本质是也是符号，只是内置函数做了特殊处理，标记位改成了 `TAG_OTHER` 即 `TYPE_BUILTIN`
2. 为何内置函数中有了，还要定义符号？没办法一次性处理，转成符号方便后续处理
3. `QUASIQUOTE` 为何使用 **lisp 实现**？逻辑复杂，使用 lisp 实现更为简介

``` lisp

; 可变参数（使用 & 捕获剩余参数）
(def sum-all (fn (& xs) (fold + 0 xs)))
(print (sum-all 1 2 3 4))               ; 输出: 10
```


## 系统库常用函数

除了 27 个内置功能，`system.lsp` 还定义了一些常用函数和宏：

| 名称 | 说明 |
|------|------|
| `defun` | 宏，定义函数：`(defun name args body)` |
| `defmacro` | 宏，定义宏：`(defmacro name args body)` |
| `if` | 宏，条件判断：`(if cond then else)` |
| `list` | 函数，创建列表：`(list a b c)` |
| `atom?` | 函数，判断是否为原子（非列表） |
| `null?` | 函数，判断是否为空列表 |
| `map` | 函数，映射：`(map f '(1 2 3))` |
| `let` | 宏，局部绑定：`(let ((x 1)) body)` |
| `fold` | 函数，左折叠 |
| `fold1` | 函数，以首元素为初值的左折叠 |
| `append` | 函数，拼接两个列表 |
| `any?` | 函数，任一元素满足谓词 |
| `zip` | 函数，多个列表拉链合并 |
| `quasiquote` | 宏，准引用（`` ` `` 语法的底层实现） |


### 宏

#### `defmacro`

```lisp
(defmacro name (params & body) body...)
```
定义宏。展开为 `(def name (macro params body))`。

#### `defun`

```lisp
(defun name (params) body...)
```
定义函数。展开为 `(def name (fn params body))`。

#### `if`

```lisp
(if cond then-expr else-expr?)   ; else-expr 可选
```
条件判断。展开为 `cond` 形式。

#### `let`

```lisp
(let ((var val) ...) body...)
```
局部绑定。展开为 `((fn (var ...) body...) val ...)`。

### 函数

#### `list`

```lisp
(list args...)
```
将参数打包为列表。

#### `atom?`

```lisp
(atom? v)
```
是否为原子（非列表）。

#### `null?`

```lisp
(null? l)
```
空列表判断。

#### `map`

```lisp
(map f lst)
```
映射函数到列表每个元素。

#### `fold`

```lisp
(fold f init lst)
```
左折叠。

#### `fold1`

```lisp
(fold1 f lst)
```
以列表首元素为初始值的左折叠。

#### `append`

```lisp
(append l1 l2)
```
拼接两个列表。

#### `any?`

```lisp
(any? pred lst)
```
是否存在元素满足谓词。

#### `fst`, `snd`

```lisp
(fst l)     → 取列表第一个元素
(snd l)     → 取列表第二个元素
```

#### `zip`

```lisp
(zip l1 l2 ...)
```
将多个列表按位置配对：
```lisp
(zip '(1 2 3) '(a b c))  →  ((1 a) (2 b) (3 c))
```


## 宏系统

宏在编译期展开，操作未求值的源代码。宏定义使用 `macro` 关键字：

```lisp
(defmacro twice (expr)
  (list 'do expr expr))

(twice (print 'hello))
; 展开为: (do (print 'hello) (print 'hello))
```

宏函数体在展开时自动获得 `tail_macro` 标记，返回的表达式会继续被展开（宏展开是迭代的，直到结果不再是宏调用）。

### 可变参数

使用 `&` 符号捕获剩余参数：

```lisp
(defun vari-sum (& xs)
  (fold + 0 xs))

(vari-sum 1 2 3)  → 6
```


## 作用域与闭包

- **全局环境**：用 `def` 定义，持久绑定。
- **词法环境**：`fn` 创建闭包时，通过**深拷贝捕获**（copy-body）捕获当前环境中的自由变量值。
- **环境栈**：`g_env_stack` 以 `[值, 符号名, 值, 符号名, ...]` 形式组织，符号查找时从栈顶向下匹配。

```lisp
(defun make-adder (x)
  (fn (y) (+ x y)))

(def add5 (make-adder 5))
(add5 10)  → 15
```


## 尾递归优化

解释器通过 `tail_eval` 宏实现尾递归调用优化，避免栈溢出：

```lisp
(defun sum-to (n acc)
  (if (= n 0) acc
    (sum-to (- n 1) (+ acc n))))

(sum-to 100000 0)  ; 不会栈溢出
```


## 错误处理

在 REPL 模式下，错误通过 `setjmp`/`longjmp` 捕获，不会退出解释器：

```
> (1 2)
Error: Applying not a function
>
```

在脚本模式下，错误调用 `exit(1)` 终止程序。


## 内存管理

### GC（垃圾回收）

- **堆**：初始大小 64KB，满时按 1.5x 比例增长
- **触发条件**：`halloc` 分配时堆空间不足
- **算法**：Cheney 半区复制（stop-and-copy）
  - 将根集（栈、环境栈、符号表）中的存活对象从 `g_heap` 复制到 `g_newheap`
  - 复制完成后交换两个半区的角色
  - 旧半区填充 `0x0A` 以暴露野指针
- **根集**：`g_stack`（值栈），`g_env_stack`（环境栈），`symtab`（符号表绑定值）

### 栈

- **值栈** `g_stack`：160K 元素
- **环境栈** `g_env_stack`：160K 元素
