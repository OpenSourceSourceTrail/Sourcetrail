# ${CMAKE_SOURCE_DIR}/src/lib_utility/tests/CMakeLists.txt
set(test_lib_names UtilityStringTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC GTest::gtest
           GTest::gmock
           Sourcetrail::gtest_main
           Sourcetrail::lib_utility
           $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
           $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  gtest_discover_tests(${test_name} TEST_PREFIX "unittests.lib_utility.")
endforeach()
