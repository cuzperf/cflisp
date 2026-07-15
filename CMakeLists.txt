cmake_minimum_required(VERSION 3.16)

project(
  cflisp
  VERSION 0.0
  DESCRIPTION "Learn lisp for fun"
  LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)         # 方便 VSCode 代码跳转
set_property(GLOBAL PROPERTY USE_FOLDERS ON)  # 方便 VS 调试

include(${CMAKE_SOURCE_DIR}/cmake/options.cmake)  # C/C++ 编译选项
include(${CMAKE_SOURCE_DIR}/cmake/coverage.cmake) # 覆盖度配置

# add lisp
include(${CMAKE_SOURCE_DIR}/cmake/cflisplib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/cflisp.cmake)

# add test
include(${CMAKE_SOURCE_DIR}/cmake/gtest.cmake)
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)

# 设置启动项(VS 特有配置)
if (MSVC)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT cflisp)
endif()

# 剩余工程为 CMAKE 预定义工程
set(CMAKE_FOLDER "CMakePredefinedTargets")
