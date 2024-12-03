# ${CMAKE_SOURCE_DIR}/src/test/CMakeLists.txt

add_subdirectory(helper)
# ========================================================
set(test_lib_names
    GraphTestSuite
    HierarchyCacheTestSuite
    IndexerCompositeTestSuite
    NetworkProtocolHelperTestSuite
    ProjectSettingsTestSuite
    ResourcePathsTestSuite
    SearchIndexTestSuite
    SettingsMigratorTestSuite
    SettingsTestSuite
    SharedMemoryTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC lib::mocks
           GTest::gmock
           GTest::gtest
           Sourcetrail::gtest_main
           Sourcetrail::core::utility::utility
           Sourcetrail::core::utility::TextAccess
           Sourcetrail::lib)

  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  gtest_discover_tests(
    ${test_name}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
    TEST_PREFIX "unittests.")
endforeach()
# ========================================================
