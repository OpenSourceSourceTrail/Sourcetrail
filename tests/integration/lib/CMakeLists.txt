# ${CMAKE_SOURCE_DIR}/tests/integration/lib/CMakeLists.txt

set(test_lib_names
    RefreshInfoGeneratorTestSuite
    SourceGroupTestSuite
    SqliteBookmarkStorageTestSuite
    SqliteIndexStorageTestSuite
    StorageTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC GTest::gtest
           GTest::gmock
           Qt5::Core
           Qt5::Gui
           Sourcetrail::messaging
           Sourcetrail::gtest_main
           lib::mocks
           Boost::date_time
           $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
           $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
           $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  # TODO(Hussein): Missing from some library
  target_link_libraries(${test_name} PRIVATE Boost::date_time)

  gtest_discover_tests(
    ${test_name}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
    TEST_PREFIX "integration.")
endforeach()

target_link_libraries(SourceGroupTestSuite PRIVATE Boost::date_time # TODO(Hussein): Missing from some library
)
