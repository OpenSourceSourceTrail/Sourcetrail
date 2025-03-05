# Sourcetrail CMake Functions
#
# This file contains utility functions for creating Sourcetrail test
# with standardized configurations.

# Function to add a Sourcetrail test executable with standardized configuration
#
# This function creates a test executable with all necessary dependencies, settings,
# and GTest integration for Sourcetrail tests.
#
# Usage:
#   add_sourcetrail_test(
#     NAME <test_name>
#     [SOURCES <source_files...>]
#     [DEPS <dependencies...>]
#     [TEST_PREFIX <prefix>]
#     [WORKING_DIRECTORY <directory>]
#   )
#
# Parameters:
#   NAME (required):
#     Name of the test executable
#     Example: NAME my_component_test
#
#   SOURCES (optional):
#     List of source files for the test
#     If not provided, defaults to ${NAME}.cpp
#     Example: SOURCES test1.cpp test2.cpp
#
#   DEPS (optional):
#     Additional public dependencies beyond the standard test dependencies
#     Example: DEPS my_extra_lib another_dependency
#
#   TEST_PREFIX (optional):
#     Prefix for test discovery
#     Defaults to "unittests.lib."
#     Example: TEST_PREFIX "custom.tests."
#
#   WORKING_DIRECTORY (optional):
#     Working directory for test execution
#     Defaults to "${CMAKE_BINARY_DIR}/test/"
#     Example: WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/custom_tests/"
#
# Standard dependencies automatically included:
#   - lib_test_utilities
#   - lib::mocks
#   - GTest::gmock
#   - GTest::gtest
#   - Sourcetrail::lib
#   - Sourcetrail::gtest_main
#   - Qt5::Gui
#   - sanitizer::address (if ENABLE_SANITIZER_ADDRESS is ON)
#   - sanitizer::undefined (if ENABLE_SANITIZER_UNDEFINED_BEHAVIOR is ON)
#
# Example usage:
#   # Basic usage with default settings
#   add_sourcetrail_test(
#     NAME my_test
#   )
#
#   # Advanced usage with custom settings
#   add_sourcetrail_test(
#     NAME advanced_test
#     SOURCES
#       advanced_test.cpp
#       test_helpers.cpp
#     DEPS
#       my_custom_library
#       internal_test_helpers
#     TEST_PREFIX "custom.advanced."
#     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/advanced_tests/"
#   )
#
function(add_sourcetrail_test)
  # Define the expected arguments
  set(options "")
  set(oneValueArgs
      NAME # Name of the test executable
      TEST_PREFIX # Prefix for test discovery
      WORKING_DIRECTORY # Working directory for tests
  )
  set(multiValueArgs SOURCES # Source files
                     DEPS # dependencies
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

  if(NOT DEFINED ARG_SOURCES)
    message(FATAL_ERROR "SOURCES argument is required")
  endif()

  # Set default values
  if(NOT DEFINED ARG_TEST_PREFIX)
    message(FATAL_ERROR "TEST_PREFIX argument is required")
  endif()

  if(NOT DEFINED ARG_WORKING_DIRECTORY)
    set(ARG_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/")
  endif()

  # Add the test executable
  add_executable(${ARG_NAME})

  target_sources(${ARG_NAME} PRIVATE ${ARG_SOURCES})

  # Set standard test dependencies
  set(STANDARD_DEPS Sourcetrail::gtest_main)

  # Add sanitizer dependencies conditionally
  list(
    APPEND
    STANDARD_DEPS
    $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
    $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  # Link all dependencies
  target_link_libraries(${ARG_NAME} PRIVATE ${STANDARD_DEPS} ${ARG_DEPS})

  # Set runtime output directory
  set_target_properties(${ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ARG_WORKING_DIRECTORY}")

  # Configure test discovery
  gtest_discover_tests(
    ${ARG_NAME}
    WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}" DISCOVERY_MODE PRE_TEST
    TEST_PREFIX "${ARG_TEST_PREFIX}")
endfunction()
