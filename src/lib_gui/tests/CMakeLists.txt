# ${CMAKE_SOURCE_DIR}/src/lib_gui/tests/CMakeLists.txt
set(test_names globalStringsTestSuite utilityAppTestSuite)

foreach(test_name IN LISTS test_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC lib::mocks
           GTest::gmock
           GTest::gtest
           Sourcetrail::gtest_main
           Sourcetrail::lib
           Sourcetrail::lib_gui
           $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
           $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
           $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  gtest_discover_tests(
    ${test_name}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
    TEST_PREFIX "unittests.lib_gui.")
endforeach()

if(ENABLE_GUI_TEST)
  add_subdirectory(gui)
endif()
