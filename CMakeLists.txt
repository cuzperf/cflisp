cmake_minimum_required(VERSION 3.16)


project(
  cflisp
  VERSION 0.0
  DESCRIPTION "Learn lisp for fun"
  LANGUAGES C CXX
)

if (MSVC)
  add_compile_options(/W4)      # 警告等级4
  add_compile_options(/wd4819)  # 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失
else()
  add_compile_options(-Wall -Wextra)        # 开启警告信息
  add_compile_options(-fvisibility=hidden)  # 默认符号隐藏
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# add lisp

add_library(cflisplib SHARED
  src/lisp.h
  src/lisp_internal.h
  src/lisp_core.c
  src/lisp_eval.c
  src/lisp_list.c
  src/lisp_print.c
  src/lisp_read.c
  src/lisp_repl.c
  src/lisp_symbol.c
)
target_compile_definitions(cflisplib PRIVATE BUILDING_CF_LIB)

# VS 特有配置
if (MSVC)
  # 将 example 中的示例文件加入到工程中
  file(GLOB Examples "${CMAKE_SOURCE_DIR}/examples/*")
  set_source_files_properties(${Examples} PROPERTIES HEADER_FILE_ONLY TRUE)
  source_group("examples" FILES ${Examples})

  # 将 system.lsp 加入到工程
  set_source_files_properties(system.lsp PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

add_executable(cflisp src/lisp_main.c system.lsp ${Examples})
target_link_libraries(cflisp cflisplib)

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

# VS 特有配置
if (MSVC)
  # 设置启动项
  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT cflisp)
  # 设置工作目录
  set_property(TARGET cflisp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  if (ExampleID)
    set_property(TARGET cflisp PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "${CMAKE_SOURCE_DIR}/examples/${ExampleID}")
  endif()
endif()

include(${CMAKE_SOURCE_DIR}/cmake/stack_size.cmake)
set_stack_size(cflisp ${STACK_SIZE_16M})

# coverage
include(${CMAKE_SOURCE_DIR}/cmake/coverage.cmake)

# gtest
set(GTEST_DIR ${CMAKE_SOURCE_DIR}/third_party/googletest-1.15.2)
add_subdirectory(${GTEST_DIR})
set_property(TARGET gtest PROPERTY FOLDER "GoogleTest")
set_property(TARGET gtest_main PROPERTY FOLDER "GoogleTest")
set_property(TARGET gmock PROPERTY FOLDER "GoogleTest")
set_property(TARGET gmock_main PROPERTY FOLDER "GoogleTest")

# add test
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)

set(CMAKE_FOLDER "CMakePredefinedTargets")
