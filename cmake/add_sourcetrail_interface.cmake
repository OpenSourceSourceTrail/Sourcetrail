# Sourcetrail CMake Functions
#
# This file contains utility functions for creating Sourcetrail libraries and tests
# with standardized configurations.

#------------------------------------------------------------------------------
# Function to add a Sourcetrail library with standardized configuration
#
# This function creates a library with proper namespacing, dependencies, and
# warning configurations for Sourcetrail components.
#
# Usage:
#   add_sourcetrail_interface(
#     NAME <library_name>
#     [DEPS <dependencies...>]
#   )
#
# Parameters:
#   NAME (required):
#     Name of the library, including namespace
#     Example: NAME lib::data::storage::SQLiteStorage
#
#   DEPS (optional):
#     Public dependencies required by the library's public API
#     Example: PUBLIC_DEPS nonstd::expected-lite
#
# Example usage:
#   # Basic library
#   add_sourcetrail_interface(
#     NAME lib::data::BasicStorage
#   )
#
#   # Complex library with dependencies
#   add_sourcetrail_interface(
#     NAME lib::data::storage::SQLiteStorage
#     DEPS
#       sqlite3::sqlite3
#   )
function(add_sourcetrail_interface)
  # Define the expected arguments
  set(options "")
  # Base name of the library
  set(oneValueArgs NAME)
  set(multiValueArgs DEPS # dependencies
  )

  # Parse the arguments
  cmake_parse_arguments(
    ARG
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN})

  # Validate required arguments
  if(NOT DEFINED ARG_NAME)
    message(FATAL_ERROR "NAME argument is required")
  endif()

  if(NOT
     ARG_NAME
     MATCHES
     "^[a-zA-Z0-9_]+::[a-zA-Z0-9_:]+$")
    message(FATAL_ERROR "Invalid library name format: ${ARG_NAME}")
  endif()

  # 2. Validate source and header files
  foreach(source ${ARG_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
      message(FATAL_ERROR "Source file not found: ${source}")
    endif()
  endforeach()

  foreach(header ${ARG_PUBLIC_HEADERS} ${ARG_PRIVATE_HEADERS})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
      message(FATAL_ERROR "Header file not found: ${header}")
    endif()
  endforeach()

  # Create the actual library name with the full namespace
  string(
    REPLACE "::"
            "_"
            LIBRARY_NAME
            "Sourcetrail_${ARG_NAME}")

  # Add the library
  add_library(${LIBRARY_NAME} INTERFACE)

  # Create the aliased target name with proper namespacing
  string(
    REPLACE "_"
            "::"
            ALIAS_NAME
            "Sourcetrail::${ARG_NAME}")
  add_library(${ALIAS_NAME} ALIAS ${LIBRARY_NAME})

  # Set include directories
  target_include_directories(${LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

  # Add dependencies
  if(ARG_DEPS)
    target_link_libraries(${LIBRARY_NAME} INTERFACE ${ARG_DEPS})
  endif()

  myproject_set_project_warnings(
    ${LIBRARY_NAME}
    ${SOURCETRAIL_WARNING_AS_ERROR}
    ""
    ""
    ""
    "")
endfunction()
