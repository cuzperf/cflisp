project(test)

enable_testing()

include_directories("${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include")
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/tests")

# test_utils
if (MSVC)
  # 忽略 sprintf 不安全警告
  set_source_files_properties(utils/test_lisp_eval.c PROPERTIES COMPILE_FLAGS /wd4996)
endif()
add_library(test_utils
  utils/string_to_file.c
  utils/test_lisp_eval.c
  test_utils.h
)

# unit_test
add_executable(unit_test
  ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/src/gtest_main.cc

  unit_test/test_cflisp.cpp
)
set_target_properties(unit_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
target_link_libraries(unit_test test_utils gtest cflisplib)

# VS 特有配置
if (MSVC)
  # 设置工作目录
  set_property(TARGET unit_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
