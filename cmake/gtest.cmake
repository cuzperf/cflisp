# add gtest
set(GTEST_DIR ${CMAKE_SOURCE_DIR}/third_party/googletest-1.15.2)
add_subdirectory(${GTEST_DIR})
set_property(TARGET gtest PROPERTY FOLDER "GoogleTest")
set_property(TARGET gtest_main PROPERTY FOLDER "GoogleTest")
set_property(TARGET gmock PROPERTY FOLDER "GoogleTest")
set_property(TARGET gmock_main PROPERTY FOLDER "GoogleTest")
