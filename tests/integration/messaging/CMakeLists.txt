# ${CMAKE_SOURCE_DIR}/tests/integration/messaging/CMakeLists.txt
if(WIN32)
  set(test_lib_names MessageQueueTestSuite)

  foreach(test_name IN LISTS test_lib_names)
    add_executable(${test_name} ${test_name}.cpp)

    target_link_libraries(
      ${test_name}
      PUBLIC GTest::gtest
             Sourcetrail::messaging
             Sourcetrail::gtest_main
             $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
             $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

    gtest_discover_tests(
      ${test_name}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
      TEST_PREFIX "integration.")
  endforeach()
endif()
