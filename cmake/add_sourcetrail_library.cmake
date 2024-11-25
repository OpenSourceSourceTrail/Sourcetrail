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
#   add_sourcetrail_library(
#     NAME <library_name>
#     [SOURCES <source_files...>]
#     [PUBLIC_HEADERS <header_files...>]
#     [PRIVATE_HEADERS <header_files...>]
#     [PUBLIC_DEPS <dependencies...>]
#     [PRIVATE_DEPS <dependencies...>]
#     [WARNING_AS_ERROR <ON|OFF>]
#   )
#
# Parameters:
#   NAME (required):
#     Name of the library, including namespace
#     Example: NAME lib::data::storage::SQLiteStorage
#
#   SOURCES (optional):
#     List of source files
#     Example: SOURCES SQLiteStorage.cpp Implementation.cpp
#
#   PUBLIC_HEADERS (optional):
#     Header files that are part of the library's public API
#     Example: PUBLIC_HEADERS SQLiteStorage.hpp
#
#   PRIVATE_HEADERS (optional):
#     Header files that are internal to the library
#     Example: PRIVATE_HEADERS Implementation.hpp
#
#   PUBLIC_DEPS (optional):
#     Public dependencies required by the library's public API
#     Example: PUBLIC_DEPS nonstd::expected-lite
#
#   PRIVATE_DEPS (optional):
#     Dependencies used only in the implementation
#     Example: PRIVATE_DEPS internal::utils
#
#   WARNING_AS_ERROR (optional):
#     Whether to treat warnings as errors
#     Defaults to project setting
#     Example: WARNING_AS_ERROR ON
#
# Example usage:
#   # Basic library
#   add_sourcetrail_library(
#     NAME lib::data::BasicStorage
#     SOURCES
#       BasicStorage.cpp
#     PUBLIC_HEADERS
#       BasicStorage.hpp
#   )
#
#   # Complex library with dependencies
#   add_sourcetrail_library(
#     NAME lib::data::storage::SQLiteStorage
#     SOURCES
#       SQLiteStorage.cpp
#       Implementation.cpp
#     PUBLIC_HEADERS
#       SQLiteStorage.hpp
#     PRIVATE_HEADERS
#       Implementation.hpp
#     PUBLIC_DEPS
#       sqlite3::sqlite3
#       nonstd::expected-lite
#     PRIVATE_DEPS
#       internal::utils
#     WARNING_AS_ERROR ON
#   )
function(add_sourcetrail_library)
  # Define the expected arguments
  set(options "")
  # Base name of the library
  set(oneValueArgs NAME)
  set(multiValueArgs
      SOURCES # Source files
      PRIVATE_HEADERS # Private header files
      PUBLIC_HEADERS # Public header files
      PUBLIC_DEPS # Public dependencies
      PRIVATE_DEPS # Private dependencies
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
  add_library(${LIBRARY_NAME})

  # Create the aliased target name with proper namespacing
  string(
    REPLACE "_"
            "::"
            ALIAS_NAME
            "Sourcetrail::${ARG_NAME}")
  add_library(${ALIAS_NAME} ALIAS ${LIBRARY_NAME})

  # Add sources
  target_sources(
    ${LIBRARY_NAME}
    PRIVATE ${ARG_SOURCES} ${ARG_PRIVATE_HEADERS}
    PUBLIC ${ARG_PUBLIC_HEADERS})

  # Set include directories
  target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

  # Add dependencies
  if(ARG_PUBLIC_DEPS)
    target_link_libraries(${LIBRARY_NAME} PUBLIC ${ARG_PUBLIC_DEPS})
  endif()

  if(ARG_PRIVATE_DEPS)
    target_link_libraries(${LIBRARY_NAME} PRIVATE ${ARG_PRIVATE_DEPS})
  endif()

  myproject_set_project_warnings(
    ${LIBRARY_NAME}
    ${SOURCETRAIL_WARNING_AS_ERROR}
    ""
    ""
    ""
    "")
endfunction()
