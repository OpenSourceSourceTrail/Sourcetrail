if(ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(--coverage)
    add_link_options(--coverage)

    find_program(GCOVR_EXECUTABLE gcovr REQUIRED)

    # gcovr and gcov must match the compiler that produced the .gcda data. A
    # version mismatch makes gcovr's worker throw, so no report is generated.
    # Resolve the gcov for the compiler's major version, falling back to `gcov`.
    string(REGEX MATCH "^[0-9]+" GCOV_MAJOR_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
    find_program(GCOV_EXECUTABLE NAMES gcov-${GCOV_MAJOR_VERSION} gcov REQUIRED)

    include(ProcessorCount)
    ProcessorCount(CORES_COUNT)

    # Directories excluded from the report: every tests tree, the app entry
    # points, the vendored external code, the C/C++ indexer worker and the JVM
    # indexer. These are absolute paths -- gcovr matches them literally and they
    # are shell-safe inside add_custom_target (regexes like ^|( are not).
    set(coverage_excludes
        "${PROJECT_SOURCE_DIR}/src/lib/client/tests"
        "${PROJECT_SOURCE_DIR}/src/lib/core/tests"
        "${PROJECT_SOURCE_DIR}/src/lib/core/http/tests"
        "${PROJECT_SOURCE_DIR}/src/lib/lib/tests"
        "${PROJECT_SOURCE_DIR}/src/lib/lib_gui/tests"
        "${PROJECT_SOURCE_DIR}/src/lib/messaging/tests"
        "${PROJECT_SOURCE_DIR}/indexers/cxx/lib/tests"
        "${PROJECT_SOURCE_DIR}/tests"
        "${PROJECT_SOURCE_DIR}/src/app"
        "${PROJECT_SOURCE_DIR}/src/lib/external"
        "${PROJECT_SOURCE_DIR}/indexers/cxx/indexer"
        "${PROJECT_SOURCE_DIR}/indexers/java")

    # gcovr's default search path is --root, so where the .gcda actually live
    # (the build tree) has to be passed explicitly as a positional search path.
    set(coverage_args
        -r
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        --html-nested=${PROJECT_BINARY_DIR}/coverage/index.html
        --txt
        --exclude-unreachable-branches
        --exclude-throw-branches
        --gcov-ignore-errors=no_working_dir_found
        --gcov-ignore-parse-errors=negative_hits.warn_once_per_file
        --gcov-executable
        ${GCOV_EXECUTABLE}
        --gcov-delete
        --sort-percentage
        -j
        ${CORES_COUNT})
    foreach(exclude IN LISTS coverage_excludes)
      list(APPEND coverage_args -e ${exclude})
    endforeach()

    # Build coverage: run the tests (so .gcda counters exist), then turn them
    # into an HTML report under ${PROJECT_BINARY_DIR}/coverage/.
    add_custom_target(
      coverage
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/coverage
      COMMAND ${GCOVR_EXECUTABLE} ${coverage_args}
      COMMENT "Generate coverage report"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  else()
    message(FATAL_ERROR "Coverage is only supported with GNU; compiler ${CMAKE_CXX_COMPILER_ID} is not supported")
  endif()
endif()
