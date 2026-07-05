option(COVERAGE "Use gcov" OFF)
message(STATUS COVERAGE=${COVERAGE})

if(COVERAGE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  set(LCOV_REMOVE_EXTRA
    "/usr/include/*"
    "/usr/include/c++/*/*"
    "/usr/include/c++/*/*/*"
    "/usr/include/x86_64-linux-gnu/c++/*/*"
    "/usr/include/x86_64-linux-gnu/c++/*/*/*"
  )
  if(CMAKE_VERBOSE_MAKEFILE)
    message(STATUS " LCOV_REMOVE_EXTRA=${LCOV_REMOVE_EXTRA}")
  endif()
endif()

set(THIRD_PARTY_FILES "*/third_party/*")
set(TEST_FILES "*/tests/*")
set(IGNORE_FILES1 "*/src/lisp_main.c")
set(IGNORE_FILES2 "*/src/lisp_repl.c")
set(IGNORE_FILES3 "*/src/lisp_dump.c")

# 检测 lcov 版本，动态设置参数
execute_process(
    COMMAND lcov --version
    OUTPUT_VARIABLE LCOV_VERSION_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 提取版本号（例如 "lcov: LCOV version 1.16"）
string(REGEX MATCH "([0-9]+\\.[0-9]+)" LCOV_VERSION "${LCOV_VERSION_OUTPUT}")

if(LCOV_VERSION VERSION_GREATER_EQUAL "2.0")
  set(LCOV_IGNORE_ERRORS "--ignore-errors;unused")
else()
  set(LCOV_IGNORE_ERRORS "--ignore-errors;source,gcov")
endif()

message(STATUS " LCOV_VERSION=${LCOV_VERSION_OUTPUT}")
message(STATUS " LCOV_IGNORE_ERRORS=${LCOV_IGNORE_ERRORS}")

# Generate coverage data
add_custom_target(coverage
  COMMAND lcov ${LCOV_IGNORE_ERRORS} --directory . --capture --exclude "${THIRD_PARTY_FILES}" --exclude "${TEST_FILES}" --exclude "${IGNORE_FILES1}" --exclude "${IGNORE_FILES2}" --exclude "${IGNORE_FILES3}" --output-file coverageall.info
  COMMAND lcov ${LCOV_IGNORE_ERRORS} --remove coverageall.info ${LCOV_REMOVE_EXTRA} --output-file coverage.info
  COMMAND genhtml coverage.info --output-directory coverage
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
