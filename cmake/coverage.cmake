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

# Generate coverage data
add_custom_target(coverage
  COMMAND lcov --ignore-errors unused --directory . --capture --exclude "${THIRD_PARTY_FILES}" --exclude "${TEST_FILES}" --output-file coverageall.info
  COMMAND lcov --ignore-errors unused --remove coverageall.info ${LCOV_REMOVE_EXTRA} --output-file coverage.info
  COMMAND genhtml coverage.info --output-directory coverage
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
