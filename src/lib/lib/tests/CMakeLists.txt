# ${PROJECT_SOURCE_DIR}/src/lib/tests/CMakeLists.txt
add_subdirectory(mocks)
add_subdirectory(helper)

add_library(lib_test_utilities utilities/FileHandler.cpp)

target_include_directories(lib_test_utilities PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(lib_test_utilities PRIVATE Sourcetrail::warnings)

set(test_lib_names
    AppPathTestSuite
    ApplicationTestSuite # TODO(Hussein): Move to integration-tests
    CodeControllerTestSuite
    ComponentFactoryTestSuite
    ComponentManagerTestSuite
    ComponentTestSuite
    ConvertGraphTestSuite
    ConvertLocationsTestSuite
    ConvertQueryTestSuite
    DialogViewTestSuite
    FactoryTestSuite
    FileHandlerTestSuite
    GraphControllerTestSuite
    GraphTestSuite
    GraphViewStyleTestSuite # TODO(SOUR-97)
    HierarchyCacheTestSuite
    IDECommunicationControllerTestSuite
    IncludeDirectiveTestSuite
    utilitySourceGroupCxxTestSuite
    IndexerCommandJavaTestSuite
    IndexerWireEnumsTestSuite
    IndexerCompositeTestSuite
    IndexerPluginRegistryTestSuite
    IndexerWorkerServiceTestSuite
    IntermediateStorageTestSuite
    LanguagePackageManagerTestSuite
    LocationTypeTestSuite
    NetworkProtocolHelperTestSuite
    ProjectSettingsTestSuite
    ProjectTestSuite
    ResourcePathsTestSuite
    SettingsMigratorTestSuite
    SettingsTestSuite
    SharedMemoryTestSuite
    SingleValueCacheTestSuite
    SourceLocationCollectionTestSuite
    SourceLocationFileTestSuite
    SourceLocationTestSuite
    StorageCacheTestSuite
    StorageProviderTestSuite
    TabIdTestSuite
    TabTestSuite
    TaskBuildIndexTestSuite
    TimeStampTestSuite
    TreeTestSuite
    UserPathsTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_sourcetrail_test(
    NAME ${test_name}
    SOURCES ${test_name}.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)
endforeach()

foreach(test_name IN LISTS test_lib_names)
  target_link_libraries(${test_name} PRIVATE Sourcetrail::lib::proto::convert)
endforeach()

target_link_libraries(LanguagePackageManagerTestSuite PRIVATE Sourcetrail::lib::core::utility::ScopedFunctor)

target_link_libraries(SingleValueCacheTestSuite PRIVATE Sourcetrail::lib::core::utility::SingleValueCache)

target_link_libraries(IncludeDirectiveTestSuite PRIVATE Sourcetrail::lib::core::utility::file::ScopedTemporaryFile)


if(WIN32)
  target_compile_options(ComponentManagerTestSuite PRIVATE "/bigobj")
endif()
# ================================================================================================================================
