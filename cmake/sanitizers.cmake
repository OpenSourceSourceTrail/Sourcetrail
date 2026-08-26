if(SR_SAN)
  string(
    REPLACE ","
            ";"
            SR_SAN_LIST
            "${SR_SAN}")
  set(_sr_san_allowed
      address
      undefined
      thread
      memory
      leak)
  foreach(_san IN LISTS SR_SAN_LIST)
    if(NOT
       _san
       IN_LIST
       _sr_san_allowed)
      message(FATAL_ERROR "SR_SAN: unknown sanitizer '${_san}'. Valid: address, undefined, thread, memory, leak.")
    endif()
  endforeach()

  set(_sr_san_core address thread memory)
  set(_sr_san_requested_core "")
  foreach(_san IN LISTS SR_SAN_LIST)
    if(_san IN_LIST _sr_san_core)
      list(APPEND _sr_san_requested_core ${_san})
    endif()
  endforeach()
  list(LENGTH _sr_san_requested_core _sr_san_core_count)
  if(_sr_san_core_count GREATER 1)
    message(FATAL_ERROR "SR_SAN: address, thread and memory are mutually exclusive (got: ${_sr_san_requested_core}).")
  endif()

  if("leak" IN_LIST SR_SAN_LIST AND ("thread" IN_LIST SR_SAN_LIST OR "memory" IN_LIST SR_SAN_LIST))
    message(
      FATAL_ERROR
        "SR_SAN: leak cannot combine with thread or memory (address already includes leak detection; use address,leak or leak alone)."
    )
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "SR_SAN is not supported with MSVC.")
  endif()
  if("memory" IN_LIST SR_SAN_LIST
     AND NOT
         CMAKE_CXX_COMPILER_ID
         MATCHES
         ".*Clang")
    message(FATAL_ERROR "SR_SAN=memory requires Clang; current compiler is ${CMAKE_CXX_COMPILER_ID}.")
  endif()

  list(
    JOIN
    SR_SAN_LIST
    ","
    _sr_san_joined)
  add_compile_options(-fsanitize=${_sr_san_joined} -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${_sr_san_joined})
endif()
