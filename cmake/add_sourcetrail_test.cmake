# Declares a GTest executable and registers it with CTest.
#
#   add_sourcetrail_test(
#     NAME MyTestSuite                       # required; the target and executable name
#     SOURCES MyTestSuite.cpp                # required
#     DEPS Sourcetrail::lib::lib_engine      # on top of the two linked automatically
#     TEST_PREFIX "unittests.lib."           # required; ctest name prefix, see the registered
#                                            # prefixes in CLAUDE.md
#     LOCK filesystem                        # optional; serializes against other suites
#                                            # sharing the same name
#     WORKING_DIRECTORY <dir>)               # defaults to ${PROJECT_BINARY_DIR}/test/
#
# Sourcetrail::gtest_main and Sourcetrail::warnings are linked for you. TEST_PREFIX is required
# rather than defaulted: each test directory registers its own prefix, and a wrong one silently
# files the tests under the wrong ctest bucket.
function(add_sourcetrail_test)
  # Define the expected arguments
  set(options "")
  set(oneValueArgs
      NAME # Name of the test executable
      TEST_PREFIX # Prefix for test discovery
      WORKING_DIRECTORY # Working directory for tests
      LOCK # optional ctest RESOURCE_LOCK, for suites sharing on-disk fixtures
  )
  set(multiValueArgs SOURCES # Source files
                     DEPS # dependencies
  )

  # Parse the arguments
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

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
    set(ARG_WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/test/")
  endif()

  # Add the test executable
  add_executable(${ARG_NAME})

  target_sources(${ARG_NAME} PRIVATE ${ARG_SOURCES})

  # Set standard test dependencies
  set(STANDARD_DEPS Sourcetrail::gtest_main Sourcetrail::warnings)

  # Link all dependencies
  target_link_libraries(${ARG_NAME} PRIVATE ${STANDARD_DEPS} ${ARG_DEPS})

  # Set runtime output directory
  set_target_properties(${ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ARG_WORKING_DIRECTORY}")

  # Every suite shares one WORKING_DIRECTORY, so suites that write real files collide under
  # `ctest -j`. RESOURCE_LOCK makes ctest run everything holding the same lock name serially,
  # while the rest of the suite still runs in parallel.
  set(discovery_properties "")
  if(DEFINED ARG_LOCK)
    set(discovery_properties PROPERTIES RESOURCE_LOCK "${ARG_LOCK}")
  endif()

  # Configure test discovery
  gtest_discover_tests(
    ${ARG_NAME}
    WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}" DISCOVERY_MODE PRE_TEST
    TEST_PREFIX "${ARG_TEST_PREFIX}" ${discovery_properties})
endfunction()
