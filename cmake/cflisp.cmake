# VS 特有配置
if (MSVC)
  # 将 example 中的示例文件加入到工程中
  file(GLOB Examples "${CMAKE_SOURCE_DIR}/examples/*")
  set_source_files_properties(${Examples} PROPERTIES HEADER_FILE_ONLY TRUE)
  source_group("examples" FILES ${Examples})

  # 将 system.lsp 加入到工程
  set_source_files_properties(system.lsp PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

add_executable(cflisp
  ${CMAKE_SOURCE_DIR}/src/lisp_main.c
  ${CMAKE_SOURCE_DIR}/system.lsp
  ${Examples})
target_link_libraries(cflisp cflisplib)

include(${CMAKE_SOURCE_DIR}/cmake/stack_size.cmake)
set_stack_size(cflisp ${STACK_SIZE_64M})

# VS 特有配置
if (MSVC)
  # 设置工作目录
  set_property(TARGET cflisp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  if (ExampleID)
    set_property(TARGET cflisp PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "${CMAKE_SOURCE_DIR}/examples/${ExampleID}")
  endif()
endif()

