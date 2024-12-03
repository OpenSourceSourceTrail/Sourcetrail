# ${CMAKE_SOURCE_DIR}/tests/integration/lib_cxx/CMakeLists.txt
set(test_cxx_names CxxIncludeProcessingTestSuite CxxParserTestSuite CxxTypeNameTestSuite)

foreach(test_name IN LISTS test_cxx_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC GTest::gtest
           GTest::gmock
           GTest::Main
           Sourcetrail::lib
           # Sourcetrail::gtest_main
           Sourcetrail::core::utility::TextAccess
           Sourcetrail::helper::TestFileRegister
           Sourcetrail::helper::TestStorage
           Sourcetrail::lib_cxx
           $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
           $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  gtest_discover_tests(
    ${test_name}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
    TEST_PREFIX "integration.lib_cxx.")
endforeach()
