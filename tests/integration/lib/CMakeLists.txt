# ${CMAKE_SOURCE_DIR}/tests/integration/lib/CMakeLists.txt

set(test_lib_names
    RefreshInfoGeneratorTestSuite
    SourceGroupTestSuite
    PersistentStorageTestSuite
    SqliteBookmarkStorageTestSuite
    SqliteDatabaseIndexTestSuite
    SqliteIndexStorageTestSuite
    SqliteStorageTestSuite
    StorageTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_sourcetrail_test(
    NAME
    ${test_name}
    SOURCES
    ${test_name}.cpp
    DEPS
    GTest::gtest
    GTest::gmock
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
    Sourcetrail::messaging
    Sourcetrail::gtest_main
    Sourcetrail::core::utility::TextAccess
    Sourcetrail::core::utility::file::FileSystem
    Sourcetrail::core::utility::file::utilityFile
    $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
    lib::mocks
    Boost::date_time
    spdlog::spdlog
    TEST_PREFIX
    "integration.lib."
    WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}/test/")
endforeach()
