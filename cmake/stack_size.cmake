# 设置目标栈大小（支持 MSVC 和 MinGW/GCC）

macro(set_stack_size target stack_size_bytes)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "set_stack_size: '${target}' is not a valid target")
  endif()

  if(WIN32)
    if(MSVC)
      target_link_options(${target} PRIVATE /STACK:${stack_size_bytes})
    elseif(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_link_options(${target} PRIVATE -Wl,--stack,${stack_size_bytes})
    else()
      message(WARNING "set_stack_size: unsupported compiler for target '${target}'")
    endif()
  elseif(APPLE)
    # macOS: Apple ld 使用 -stack_size，必须是十六进制
    math(EXPR stack_size_hex "${stack_size_bytes}" OUTPUT_FORMAT HEXADECIMAL)
      target_link_options(${target} PRIVATE -Wl,-stack_size,${stack_size_hex})
  else()
    # Linux/macOS：链接器不支持设置栈大小
    # 如需大栈，请在 CI 中使用 ulimit -s 或在代码中用 pthread_attr_setstacksize
    message(STATUS "set_stack_size: ${target} on non-Windows platform, stack size must be set via ulimit or pthread_attr")
  endif()
endmacro()

set(STACK_SIZE_64M 67108864)
