# ${CMAKE_SOURCE_DIR}/tests/integration/lib/CMakeLists.txt

set(test_lib_names
    RefreshInfoGeneratorTestSuite
    SourceGroupTestSuite
    PersistentStorageTestSuite
    SqliteBookmarkStorageTestSuite
    SqliteDatabaseIndexTestSuite
    SqliteIndexStorageTestSuite
    #SqliteStorageTestSuite
    StorageTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_sourcetrail_test(
    NAME
    ${test_name}
    SOURCES
    ${test_name}.cpp
    DEPS
    Qt5::Core
    Qt5::Gui
    Qt5::Sql
    Sourcetrail::messaging
    lib::mocks
    Boost::date_time
    $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
    TEST_PREFIX
    "integration."
    WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}/test/")
endforeach()

add_sourcetrail_test(
  NAME
  ReadSqliteBookmarkStorageTestSuite
  SOURCES
  ReadSqliteBookmarkStorageTestSuite.cpp
  DEPS
  Sourcetrail::lib
  Sourcetrail::lib::data::storage::sqlite::impls::ReadSqliteBookmarkStorage
  Sourcetrail::lib::data::storage::sqlite::factory::impls::SqliteFactory
  TEST_PREFIX
  "integration."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  SqliteFactoryTestSuite
  SOURCES
  SqliteFactoryTestSuite.cpp
  DEPS
  Sourcetrail::lib::data::storage::sqlite::factory::impls::SqliteFactory
  TEST_PREFIX
  "integration."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")
