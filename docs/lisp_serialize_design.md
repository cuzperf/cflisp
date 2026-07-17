# CF Lisp 序列化/反序列化设计文档

## 1. 目标

将 `system.lsp` 加载执行后的解释器状态序列化为二进制镜像，启动时优先从镜像加载以加速初始化；若镜像不存在或损坏则回退到 `system.lsp` 源文件加载。保证反序列化后功能与 `system.lsp` 源码加载完全一致。

## 2. 状态分析

解释器状态由以下部分组成：

| 组成 | 存储位置 | 是否需要序列化 |
|------|----------|---------------|
| 符号表 (symtab BST) | C heap (malloc) | 需要保存所有符号的 name + binding |
| GC 堆 (cons cells) | GC heap (halloc) | 需要保存所有可达的 cons cell (head, tail) |
| 值栈 (g_stack) | malloc | 不需要 (clean boundary 时为空) |
| 环境栈 (g_env_stack) | malloc | 不需要 (clean boundary 时为空) |
| 内置函数 (g_builtins[]) | 全局数组 | 不需要 (位置固定，通过 BuiltinCode 还原) |

**关键设计**：镜像在 "干净边界" 生成，即所有顶层表达式求值完毕后，`g_sp == 0` 且 `g_env_sp == 0`。此时活跃状态仅包含符号绑定和 GC 堆对象。

## 3. 镜像文件格式

### 3.1 Header

```
Offset  Size   Field
------  ----   -----
0       4      magic:  uint32_t = 0x4D444C46 ("FLDM" little-endian)
4       4      version: uint32_t = 1
8       4      num_symbols: uint32_t
12      4      num_cells: uint32_t
```

### 3.2 Value 序列化格式

每个 `value_t` 序列化为固定 8 字节：

```
uint32_t tag   (TYPE_NUM=0, TYPE_LIST=1, TYPE_SYM=2, TYPE_BUILTIN=3)
int32_t  data  (含义取决于 tag)
```

| tag | data 含义 |
|-----|----------|
| TYPE_NUM (0) | 整数值 |
| TYPE_LIST (1) | cell 数组索引 (EMPTY_LIST = -1) |
| TYPE_SYM (2) | symbol 数组索引 (UNBOUND = -1) |
| TYPE_BUILTIN (3) | BuiltinCode 枚举值 |

### 3.3 Symbol Table

```
for i = 0..num_symbols-1:
    uint16_t name_len
    char     name[name_len]      (不含 '\0')
    value_t  binding             (8 bytes, 如上格式)
```

### 3.4 Cell Table

```
for i = 0..num_cells-1:
    value_t  head                (8 bytes)
    value_t  tail                (8 bytes)
```

## 4. 序列化算法

```
cf_lisp_serialize(filename):
    1. 收集符号: 中序遍历 symtab BST → 按 hash 升序排列，分配 index
    2. 收集 Cell: 从所有符号的 binding 出发，BFS 遍历所有可达 cons cell，分配 index
    3. 写 Header
    4. 写符号表: 对每个符号，写入 name + serialized(binding)
    5. 写 Cell 表: 对每个 cell，写入 serialized(head) + serialized(tail)
```

## 5. 反序列化算法

```
cf_lisp_deserialize_image(filename):
    1. 读 Header，校验 magic/version
    2. 读符号表: 每读取一个 (name, pending_binding)，调用 symbol() 在 symtab 中查找/
       创建符号，储存 Symbol* 到临时数组
    3. 读 Cell 表: 暂存每项 head/tail 的 pending value 到临时数组
    4. 分配 Cell: 对每个 cell，调用 make_cell(UNBOUND)，push 到值栈保护
    5. 解析符号绑定: resolve 每个符号的 pending_binding → 设置 sym->binding
    6. 解析 Cell: resolve 每个 cell 的 pending head/tail → 写入 cell->head, cell->tail
    7. 弹栈，释放临时数组
```

### 5.1 GC 保护

反序列化期间，所有已分配的 Cell 都 push 在值栈上（`g_stack[cell_ss : cell_ss+num_cells)`），作为 GC 根。解析阶段不会触发 GC（仅构造 tagged pointer，不分配内存），保证了 `g_stack[]` 中 Cell 指针的有效性。

## 6. 命令行接口

```
cflisp                                    # 默认: 尝试加载 cflisp.img，失败则加载 system.lsp
cflisp --no-image                         # 跳过镜像加载，直接加载 system.lsp
cflisp --output-image <file>              # 执行完毕后序列化到 <file>
cflisp script.lsp                         # 执行脚本 (兼容已有行为)
cflisp script.lsp --output-image a.img    # 执行脚本并序列化镜像
```

## 7. 功能等价性保证

- **符号**: 所有符号（包括内置符号）按 hash 升序保存，反序列化时 `symbol()` 保证同名字符返回相同指针
- **内置函数**: 通过 `BuiltinCode` 还原，`g_builtins[]` 数组在不同运行中位置稳定
- **闭包**: 函数体中的自由变量以符号形式保存，运行时在全局符号表查找
- **自引用**: NIL/T/REST 的自引用绑定正确保存和还原
- **Cell 拓扑**: BFS 遍历确保所有可达 Cell 被完整保存

## 8. 复用的现有函数和宏

| 符号 | 用途 |
|------|------|
| `symbol()` | 创建/查找符号 |
| `make_cell()` | 在 GC 堆上分配 cons cell |
| `type_of()` | 获取值的类型 |
| `list_val()` / `sym_val()` / `builtin_val()` / `ptr()` | 解引用 tagged pointer |
| `tagptr()` / `number()` / `list()` | 构造 tagged pointer |
| `push()` / `pop()` / `restore_stack()` | 值栈操作 |
| `EMPTY_LIST` / `UNBOUND` / `NIL` / `T` | 特殊值常量 |
| `g_builtins[]` / `N_BUILTINS` | 内置函数表 |
