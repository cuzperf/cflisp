[![Code Coverage](https://github.com/cuzperf/cflisp/actions/workflows/test.yml/badge.svg)](https://github.com/cuzperf/cflisp/actions/workflows/test.yml) [![codecov](https://codecov.io/gh/cuzperf/cflisp/graph/badge.svg)](https://codecov.io/gh/cuzperf/cflisp)

# cflisp

Learn to write an lisp compiler and interpreter

[中文文档](https://cuzperf.github.io/cflisp)

## Lisp interpreter

Initial clone from <https://github.com/tanhuser/lisp-interpreter>

## build

- Windows [build.bat](build.bat)
- Linux [build.sh](build.sh)
- MacOS [buildMac.sh](buildMac.sh)

## Run

```bash
cflisp [脚本文件]
```

- **无参数**：进入 REPL 交互模式，逐条输入表达式求值
- **有参数**：执行指定脚本文件并退出

> 启动时自动加载 `system.lsp` 标准库（必须在**工作目录**）

## C API（lisp.h）

```c
void     cf_lisp_init();                    // 初始化解释器
value_t  cf_read_file(const char* name);    // 读取并解析 Lisp 文件
value_t  cf_eval_toplevel(value_t l);       // 顶层求值
void     cf_print(value_t v);               // 打印值
void     cf_println(value_t v);             // 打印值并换行
void     cf_mprint(value_t v);              // 打印值及内存布局（调试用）
void     cf_lisp_repl();                    // 启动 REPL
bool     cf_isNIL(value_t v);               // 判断是否为 nil 或空列表
number_t cf_num_val(value_t x);             // 取数值
```

## 完整示例

参见 `examples/example.lsp`，涵盖：算术、比较、类型判断、列表操作、逻辑、引号与准引用、定义、条件、函数、闭包、递归、尾递归、宏、do、eval/apply、标准库、GC 压力测试。

## 已知限制

- 仅支持整数运算（不支持浮点数），且整数有效位数为 30 位（剩余 2 位作为标记位）
- 负数需用 `(- n)` 表达，`-n` 会被解析为符号
- 不支持 Common Lisp 风格的多值返回、循环宏等
- 错误消息为英文，源码注释为中文
- 无模块/包系统
