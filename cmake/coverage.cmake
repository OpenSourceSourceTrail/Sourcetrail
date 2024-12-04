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
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
      # Run gcovr with similar exclusions as the original script
      COMMAND
        ${GCOVR_EXECUTABLE} -r ${CMAKE_SOURCE_DIR}/src/ -e ${CMAKE_SOURCE_DIR}/src/app -e
        ${CMAKE_SOURCE_DIR}/src/core/tests -e ${CMAKE_SOURCE_DIR}/src/external -e ${CMAKE_SOURCE_DIR}/src/indexer -e
        ${CMAKE_SOURCE_DIR}/src/lib/tests -e ${CMAKE_SOURCE_DIR}/src/lib_cxx/tests -e
        ${CMAKE_SOURCE_DIR}/src/lib_gui/tests -e ${CMAKE_SOURCE_DIR}/src/lib_utility/tests -e
        ${CMAKE_SOURCE_DIR}/src/messaging/tests -e ${CMAKE_SOURCE_DIR}/src/scheduling/tests -e
        ${CMAKE_SOURCE_DIR}/src/test --html-nested=${CMAKE_BINARY_DIR}/coverage/index.html --gcov-delete -j
        ${CORES_COUNT} ${CMAKE_BINARY_DIR}
      COMMENT "Generate coverage for GNU"
      # Working directory for the command
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  else()
    message(FATAL_ERROR "Compiler is not supported")
  endif()
endif()
