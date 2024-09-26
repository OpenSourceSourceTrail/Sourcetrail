# ${CMAKE_SOURCE_DIR}/src/core/tests/CMakeLists.txt

set(gtest_lib_names
    ConfigManagerTestSuite
    FileManagerTestSuite
    FilePathFilterTestSuite
    FilePathTestSuite
    FileSystemTestSuite
    TextAccessTestSuite
    VersionTestSuite
    ScopedFunctorTestSuite
    ScopedTemporaryFileTestSuite)

foreach(test_name IN LISTS gtest_lib_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC GTest::gmock
           GTest::gtest
           lib::mocks
           Sourcetrail::core
           Sourcetrail::gtest_main
           $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
           $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  gtest_discover_tests(
    ${test_name}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
    TEST_PREFIX "unittests.core.")
endforeach()

target_link_libraries(FileSystemTestSuite PRIVATE Sourcetrail::lib_utility)

target_link_libraries(FileManagerTestSuite PRIVATE Sourcetrail::lib_utility)
