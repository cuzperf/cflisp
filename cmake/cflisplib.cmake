add_library(cflisplib SHARED
  src/lisp.h
  src/lisp_internal.h
  src/lisp_core.c
  src/lisp_eval.c
  src/lisp_list.c
  src/lisp_print.c
  src/lisp_read.c
  src/lisp_repl.c
  src/lisp_serialize.c
  src/lisp_symbol.c
)
target_compile_options(cflisplib PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-g3>
)
target_compile_definitions(cflisplib PRIVATE BUILDING_CF_LIB)

# 增加编译选项
if (MSVC)
  # 部分 VS 版本 add_compile_options 不生效
  target_compile_options(cflisplib PRIVATE
    /WX         # 警告视为错误
    /wd4996     # 忽略 C4996: 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead.
    /wd4200     # 忽略 C4200: 使用了非标准扩展: 结构/联合中的零大小数组.
  )
else()
  target_compile_options(cflisplib PRIVATE
    -Werror     # 警告视为错误
  )
endif()
