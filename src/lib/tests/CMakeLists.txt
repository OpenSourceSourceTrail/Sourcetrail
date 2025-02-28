# ${CMAKE_SOURCE_DIR}/src/lib/tests/CMakeLists.txt
add_subdirectory(mocks)
add_subdirectory(helper)

add_library(lib_test_utilities utilities/FileHandler.cpp)

target_include_directories(lib_test_utilities PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set(test_lib_names
    AppPathTestSuite
    ApplicationTestSuite # TODO(Hussein): Move to integration-tests
    BookmarkControllerTestSuite
    ComponentFactoryTestSuite
    ComponentManagerTestSuite
    ComponentTestSuite
    FactoryTestSuite
    FileHandlerTestSuite
    GraphTestSuite
    GraphViewStyleTestSuite # TODO(SOUR-97)
    HierarchyCacheTestSuite
    IndexerCompositeTestSuite
    IntermediateStorageTestSuite
    LanguagePackageManagerTestSuite
    LocationTypeTestSuite
    NetworkProtocolHelperTestSuite
    ProjectSettingsTestSuite
    ProjectTestSuite
    ResourcePathsTestSuite
    SearchIndexTestSuite
    SettingsMigratorTestSuite
    SettingsTestSuite
    SharedMemoryTestSuite
    SingleValueCacheTestSuite
    SourceLocationCollectionTestSuite
    SourceLocationFileTestSuite
    SourceLocationTestSuite
    StorageCacheTestSuite
    StorageProviderTestSuite
    # StatusBarControllerTestSuite TODO(SOUR-95)
    # StatusControllerTestSuite TODO(SOUR-96)
    TabIdTestSuite
    TabTestSuite
    TimeStampTestSuite
    TreeTestSuite
    UserPathsTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_sourcetrail_test(
    NAME
    ${test_name}
    SOURCES
    ${test_name}.cpp
    DEPS
    lib_test_utilities
    lib::mocks
    Sourcetrail::lib
    Sourcetrail::gtest_main
    Sourcetrail::core::utility::TextAccess
    Qt5::Gui
    TEST_PREFIX
    "unittests.lib."
    WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}/test/")

  target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)
endforeach()

target_link_libraries(LanguagePackageManagerTestSuite PRIVATE Sourcetrail::core::utility::ScopedFunctor)

target_link_libraries(SingleValueCacheTestSuite PRIVATE Sourcetrail::core::utility::SingleValueCache)

# target_link_libraries(StatusBarControllerTestSuite PRIVATE Sourcetrail::messaging)
# target_link_libraries(StatusControllerTestSuite PRIVATE Sourcetrail::messaging)

if(WIN32)
  target_compile_options(ComponentManagerTestSuite PRIVATE "/bigobj")
endif()
# ================================================================================================================================
