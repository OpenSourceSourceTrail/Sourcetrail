if(ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(--coverage)
    add_link_options(--coverage)

    find_program(GCOVR_EXECUTABLE gcovr REQUIRED)
    find_program(GCOV_EXECUTABLE gcov REQUIRED)

    include(ProcessorCount)
    ProcessorCount(CORES_COUNT)

    # Create a custom target for code coverage
    add_custom_target(
      coverage
      # Ensure tests are run first
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      # Create coverage output directory
      COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/coverage
      # Run gcovr with similar exclusions as the original script
      COMMAND
        ${GCOVR_EXECUTABLE} -r ${PROJECT_SOURCE_DIR} -e ${PROJECT_SOURCE_DIR}/src/app -e
        ${PROJECT_SOURCE_DIR}/src/lib/core/tests -e ${PROJECT_SOURCE_DIR}/src/lib/external -e
        ${PROJECT_SOURCE_DIR}/indexers/cxx/indexer -e ${PROJECT_SOURCE_DIR}/src/lib/lib/tests -e
        ${PROJECT_SOURCE_DIR}/indexers/cxx/lib/tests -e ${PROJECT_SOURCE_DIR}/src/lib/lib_qml/tests -e
        ${PROJECT_SOURCE_DIR}/src/lib_utility/tests -e ${PROJECT_SOURCE_DIR}/src/lib/messaging/tests -e
        ${PROJECT_SOURCE_DIR}/src/lib/scheduling/tests -e ${PROJECT_SOURCE_DIR}/src/test -e ${PROJECT_SOURCE_DIR}/tests
        -e ${PROJECT_SOURCE_DIR}/indexers/java --html-nested=${PROJECT_BINARY_DIR}/coverage/index.html --gcov-delete -j
        ${CORES_COUNT} --gcov-executable ${GCOV_VERSION} --exclude-unreachable-branches --exclude-throw-branches
        ${PROJECT_BINARY_DIR}
      COMMENT "Generate coverage for GNU"
      # Working directory for the command
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  else()
    message(FATAL_ERROR "Compiler is not supported")
  endif()
endif()
