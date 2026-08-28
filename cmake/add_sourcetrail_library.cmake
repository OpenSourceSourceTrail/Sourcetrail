# Declares a Sourcetrail static library.
#
#   add_sourcetrail_library(
#     NAME lib::data::storage::SQLiteStorage   # required, "::"-separated; becomes the target
#     SOURCES SQLiteStorage.cpp Implementation.cpp
#     HEADERS SQLiteStorage.hpp                # public API; a HEADERS file set, see target_sources
#     PRIVATE_HEADERS Implementation.hpp       # internal to the library
#     PUBLIC_DEPS sqlite3::sqlite3             # reachable through the public headers
#     PRIVATE_DEPS internal::utils)            # implementation only
#
# NAME lib::data::Foo yields target Sourcetrail_lib_data_Foo and alias Sourcetrail::lib::data::Foo.
# The project warning set is linked in automatically; see cmake/compiler_warnings.cmake.
function(add_sourcetrail_library)
  # Define the expected arguments
  set(options "")
  # Base name of the library
  set(oneValueArgs NAME)
  set(multiValueArgs
      SOURCES # Source files
      PRIVATE_HEADERS # Private header files
      HEADERS # Public header files
      PUBLIC_DEPS # Public dependencies
      PRIVATE_DEPS # Private dependencies
  )

  # Parse the arguments
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Validate required arguments
  if(NOT DEFINED ARG_NAME)
    message(FATAL_ERROR "NAME argument is required")
  endif()

  if(NOT ARG_NAME MATCHES "^[a-zA-Z0-9_]+::[a-zA-Z0-9_:]+$")
    message(FATAL_ERROR "Invalid library name format: ${ARG_NAME}")
  endif()

  # Create the actual library name with the full namespace
  string(REPLACE "::" "_" LIBRARY_NAME "Sourcetrail_${ARG_NAME}")

  # Add the library
  add_library(${LIBRARY_NAME})

  # Create the aliased target name with proper namespacing
  string(REPLACE "_" "::" ALIAS_NAME "Sourcetrail::${ARG_NAME}")
  add_library(${ALIAS_NAME} ALIAS ${LIBRARY_NAME})

  # Add sources. Public headers go in a HEADERS file set rather than being listed as PUBLIC sources:
  # PUBLIC sources land in INTERFACE_SOURCES, so every consumer would inherit them as sources of its
  # own target -- which for an AUTOMOC target means moc'ing another library's headers. The file set
  # also puts BASE_DIRS on consumers' include path, so no separate target_include_directories.
  target_sources(
    ${LIBRARY_NAME}
    PRIVATE ${ARG_SOURCES} ${ARG_PRIVATE_HEADERS}
    PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ARG_HEADERS})

  # Add dependencies
  if(ARG_PUBLIC_DEPS)
    target_link_libraries(${LIBRARY_NAME} PUBLIC ${ARG_PUBLIC_DEPS})
  endif()

  if(ARG_PRIVATE_DEPS)
    target_link_libraries(${LIBRARY_NAME} PRIVATE ${ARG_PRIVATE_DEPS})
  endif()

  # PRIVATE: the warnings apply to this library's own sources and are not inflicted on consumers,
  # which link Sourcetrail::warnings themselves.
  target_link_libraries(${LIBRARY_NAME} PRIVATE Sourcetrail::warnings)
endfunction()
