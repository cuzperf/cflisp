(def else '#t) ; else 只要走到了就直接执行，所以本质上就是 true（#t 在 C 中定义）

; 这里利用了 lisp 隐藏约定 args 接受任意参数
; 注意 (fn args args) 和 (fn (args) args) 有本质的不同，后者仅接受一个参数
(def list (fn args args))

(def atom? (fn (x) (not (list? x)))) ;  非列表即为原子

; 核心作用（语法糖）：把"零个、一个或多个表达式"统一规范成合法的函数体/执行块，避免不必要的 do 包装
(def splice-body
  (fn (body)
      (cond ((atom? body) body)
      ((= (tail body) '()) (head body))
      (else (cons do body)))))

; defmacros 可以支持多行 body
(def defmacro
  (macro (name args & body)
         (list 'def name (list 'macro args (splice-body body)))))

; defun 可以支持多行 body
(defmacro defun (name args & body)
  (list 'def name (list 'fn args (splice-body body))))

; 判断是否为空列表
(defun null? (l)
  (= l '()))

; 基于 cond 定义 if
(defmacro if (c t & e)
  (cond ((null? e) (list 'cond (list c t)))
        (else (list 'cond (list c t) (list 'else (head e))))))

; 递归定义 map 行为
(defun map (f lst)
  (if (null? lst)
      lst
      (cons (f (head lst)) (map f (tail lst)))))

; (let ((var val) ...) body...) 展开为 ((fn (var ...) body...) val ...)
(defmacro let (binds & body)
  (cons (list 'fn (map head binds) (splice-body body)) (map snd binds)))

; 首子
(def fst head)

; 次子
(defun snd (l)
  (head (tail l)))

; 累记操作, s 为默认值
(defun fold (f s l)
  (if (null? l)
      s
      (fold f (f s (head l)) (tail l))))

; 对 list 进行累记操作
(defun fold1 (f l)
  (fold f (head l) (tail l)))

; l 中是否有一个满足性质 p
(defun any? (p l)
  (fold1 or (map p l)))

; 拼接列表
(defun append (l1 l2)
  (if (null? l1)
      l2
      (cons (head l1) (append (tail l1) l2))))

; 拟引用（详见 qusiquote.md）
(defmacro quasiquote (e)
  (qq-expand e))

(defun qq-expand (x)
  (if (list? x)
      (cond ((null? x) (list 'quote '()))
            ((= (head x) 'unquote) (snd x))
            ((= (head x) 'quasiquote) (qq-expand (qq-expand (snd x))))
            (else (list 'append
                    (qq-expand-list (head x))
                    (qq-expand (tail x)))))
      (list 'quote x)))

(defun qq-expand-list (x)
  (if (list? x)
      (cond ((null? x) (list 'quote (list '())))
            ((= (head x) 'unquote) (list 'list (snd x)))
            ((= (head x) 'unquote-splicing) (snd x))
            ((= (head x) 'quasiquote) (qq-expand-list (qq-expand (snd x))))
            (else (list 'list (list 'append
                                    (qq-expand-list (head x))
                                    (qq-expand (tail x))))))
      (list 'quote (list x))))

; (zip '(1 2 3) '(a b c))  →  ((1 a) (2 b) (3 c))
(defun zip l
  (if (any? null? l)
      '()
      (cons (map head l) (apply zip (map tail l)))))
