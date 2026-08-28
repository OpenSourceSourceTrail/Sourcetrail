# Declares a Sourcetrail header-only library.
#
#   add_sourcetrail_interface(
#     NAME lib::core::utility::Tree    # required, "::"-separated; becomes the target
#     DEPS nonstd::expected-lite)      # propagated to consumers
#
# NAME lib::core::Foo yields target Sourcetrail_lib_core_Foo and alias Sourcetrail::lib::core::Foo.
# No warning flags are attached: an INTERFACE library compiles nothing of its own, and adding them
# would only push this project's flags onto every consumer, which link Sourcetrail::warnings
# themselves.
function(add_sourcetrail_interface)
  cmake_parse_arguments(ARG "" "NAME" "DEPS" ${ARGN})

  if(NOT DEFINED ARG_NAME)
    message(FATAL_ERROR "NAME argument is required")
  endif()

  if(NOT ARG_NAME MATCHES "^[a-zA-Z0-9_]+::[a-zA-Z0-9_:]+$")
    message(FATAL_ERROR "Invalid library name format: ${ARG_NAME}")
  endif()

  string(REPLACE "::" "_" LIBRARY_NAME "Sourcetrail_${ARG_NAME}")
  add_library(${LIBRARY_NAME} INTERFACE)

  string(REPLACE "_" "::" ALIAS_NAME "Sourcetrail::${ARG_NAME}")
  add_library(${ALIAS_NAME} ALIAS ${LIBRARY_NAME})

  target_include_directories(${LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

  if(ARG_DEPS)
    target_link_libraries(${LIBRARY_NAME} INTERFACE ${ARG_DEPS})
  endif()
endfunction()
