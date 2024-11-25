# ${CMAKE_SOURCE_DIR}/src/core/tests/CMakeLists.txt

set(gtest_lib_names
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
# ================================================================================================================================
add_executable(ConfigManagerTestSuite ConfigManagerTestSuite.cpp)

target_link_libraries(
  ConfigManagerTestSuite
  PUBLIC Sourcetrail::gtest_main
         Sourcetrail::core
         Sourcetrail::core::utility::ConfigManager
         $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
         $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

set_target_properties(ConfigManagerTestSuite PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

gtest_discover_tests(
  ConfigManagerTestSuite
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
  TEST_PREFIX "unittests.core.")
# ================================================================================================================================
add_executable(UtilityStringTestSuite UtilityStringTestSuite.cpp)

target_link_libraries(
  UtilityStringTestSuite
  PUBLIC Sourcetrail::gtest_main
         Sourcetrail::core::utility::utilityString
         $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
         $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

set_target_properties(UtilityStringTestSuite PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

gtest_discover_tests(
  UtilityStringTestSuite
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
  TEST_PREFIX "unittests.core.")