# ${CMAKE_SOURCE_DIR}/src/test/CMakeLists.txt

add_library(testHelper helper/TestFileRegister.cpp)
target_link_libraries(testHelper PUBLIC Sourcetrail::core Sourcetrail::scheduling)
target_include_directories(testHelper PUBLIC ${CMAKE_CURRENT_LIST_DIR}/helper ${LIB_INCLUDE_PATHS})
# ========================================================
set(test_lib_names
    AppPathTestSuite
    CommandlineTestSuite
    GraphTestSuite
    HierarchyCacheTestSuite
    IndexerCompositeTestSuite
    NetworkProtocolHelperTestSuite
    ProjectSettingsTestSuite
    ResourcePathsTestSuite
    SearchIndexTestSuite
    SettingsMigratorTestSuite
    SettingsTestSuite
    SharedMemoryTestSuite
    UserPathsTestSuite
    UtilityTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC testHelper
           lib::mocks
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
