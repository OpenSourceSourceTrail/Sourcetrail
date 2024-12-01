# ${CMAKE_SOURCE_DIR}/src/lib/tests/CMakeLists.txt
add_subdirectory(mocks)

add_library(lib_test_utilities utilities/FileHandler.cpp)

target_include_directories(lib_test_utilities PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set(test_lib_names
    ApplicationTestSuite # TODO(Hussein): Move to integration-tests
    BookmarkControllerTestSuite
    ComponentFactoryTestSuite
    ComponentManagerTestSuite
    ComponentTestSuite
    FactoryTestSuite
    FileHandlerTestSuite
    LanguagePackageManagerTestSuite
    LocationTypeTestSuite
    ProjectTestSuite
    SingleValueCacheTestSuite
    SourceLocationCollectionTestSuite
    SourceLocationFileTestSuite
    SourceLocationTestSuite
    TabIdTestSuite
    TabTestSuite
    TimeStampTestSuite
    TreeTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC lib_test_utilities
           lib::mocks
           GTest::gmock
           GTest::gtest
           Sourcetrail::lib
           Sourcetrail::gtest_main
           Qt5::Gui
           $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
           $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)

  gtest_discover_tests(
    ${test_name}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
    TEST_PREFIX "unittests.lib.")
endforeach()

target_link_libraries(LanguagePackageManagerTestSuite PRIVATE Sourcetrail::core::utility::ScopedFunctor)

target_link_libraries(SingleValueCacheTestSuite PRIVATE Sourcetrail::core::utility::SingleValueCache)

if(WIN32)
  target_compile_options(ComponentManagerTestSuite PRIVATE "/bigobj")
endif()
