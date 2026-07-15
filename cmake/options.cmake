set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

if (MSVC)
  add_compile_options(/W4)      # 警告等级4
  add_compile_options(/wd4819)  # 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失
else()
  add_compile_options(-Wall -Wextra)        # 开启警告信息
  add_compile_options(-fvisibility=hidden)  # 默认符号隐藏
endif()
