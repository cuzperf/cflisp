# 困惑点自问自答

## Why lisp?

1. 无需考虑运算符优先级
2. 无需考虑结合性
3. 词法分析简单
4. 十分方便语法拓展

## gc 怎么知道对象是否存活呢

从根集合可达的对象就是存活的。在 `gc()` 中，根集合有三类：

1. 值栈 `g_stack[0..g_sp]` — 逐一 `relocate()`
2. 环境栈 `g_env_stack[0..g_env_sp]` — 按步长 2 遍历，跳过 SYM 条目，其余 `relocate()`
3. 符号表 BST — `relocate_symtab()` 递归每个符号的 binding 字段

## 频繁的 push 和 pop?

当一个操作可能触发 gc 时，那么临时变量可能失效，此时需要用 push 将变量保存，完成操作后，再 pop 取值

## 什么是尾递归优化

``` lisp
; 非尾递归：递归返回后还要做乘法
(defun fact (n)
  (if (= n 0) 1 (* n (fact (- n 1)))))

; 调 fact(4) 时栈的变化：
; fact(4) -> (* 4 (fact 3))
;            (* 4 (* 3 (fact 2)))
;            (* 4 (* 3 (* 2 (fact 1))))
;            (* 4 (* 3 (* 2 (* 1 (fact 0)))))
;            (* 4 (* 3 (* 2 (* 1 1))))  <- 开始回溯
;            (* 4 (* 3 (* 2 1)))
;            (* 4 (* 3 2))
;            (* 4 6)
; => 24
; 深度 4，每层都保留着等乘法的栈帧，空间 O(n)

; 改写成尾递归（等价于循环）：
(defun fact-tail (n acc)
  (if (= n 0) acc (fact-tail (- n 1) (* n acc))))

; 调 fact-tail(4, 1)
; -> fact-tail(3, 4)
; -> fact-tail(2, 12)
; -> fact-tail(1, 24)
; -> fact-tail(0, 24)
; => 24
; 每步都 goto 复用同一栈帧，空间 O(1)
```

- Lisp 写递归比写循环更加容易：场景多、使用广泛
- 写 lisp 函数时尽量写尾递归

## 闭包是如何实现的呢

``` lisp
(def make-adder (fn (x) (fn (y) (+ x y))))
((make-adder 5) 2)
(def add5 (make-adder 5))
(add5 2)
```

add5 是如何把 `x = 5` 保存下来的呢？

由 def 可知 add5 为符号，def 会对后面的内容进行求值 eval，然后将符号 add5 绑定给计算出来的值 `(fn (y) (+ 5 y))`

## 再分析下一段代码

```
(def y 10)
(defun fx (x) (+ x y))
(fx 3) # 返回 13
(def y 11)
(fx 3) # 返回 14
```

这段代码中 y 是一个全局符号，y 绑定的值改变后，fx 内部虽然无变化，但再次计算时，相同的输入得到了不同的输出
