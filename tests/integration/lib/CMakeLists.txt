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
    NAME ${test_name}
    SOURCES ${test_name}.cpp
    DEPS GTest::gtest
         GTest::gmock
         Qt6::Core
         Qt6::Gui
         Qt6::Sql
         Sourcetrail::lib::messaging
         Sourcetrail::gtest_main
         Sourcetrail::lib::core::utility::TextAccess
         Sourcetrail::lib::core::utility::file::FileSystem
         Sourcetrail::lib::core::utility::file::utilityFile
         $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib::lib_cxx>
         lib::mocks
         Sourcetrail::lib::lib_engine
         Boost::date_time
         spdlog::spdlog
    TEST_PREFIX "integration.lib."
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/")
endforeach()
