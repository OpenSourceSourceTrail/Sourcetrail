# ${CMAKE_SOURCE_DIR}/src/lib/tests/CMakeLists.txt
add_subdirectory(mocks)

add_library(lib_test_utilities utilities/FileHandler.cpp)

target_include_directories(lib_test_utilities PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set(test_lib_names
    ApplicationTestSuite
    BookmarkControllerTestSuite
    CommandLineParserTestSuite
    CommandlineCommandConfigTestSuite
    CommandlineCommandIndexTestSuite
    CommandlineCommandTestSuite
    CommandlineHelperTestSuite
    ComponentFactoryTestSuite
    ComponentManagerTestSuite
    ComponentTestSuite
    FileHandlerTestSuite
    LanguagePackageManagerTestSuite
    LocationTypeTestSuite
    ProjectTestSuite
    SingleValueCacheTestSuite
    SourceLocationCollectionTestSuite
    SourceLocationFileTestSuite
    SourceLocationTestSuite
    StatusTestSuite
    TabIdTestSuite
    TabTestSuite
    TimeStampTestSuite
    TreeTestSuite
    UnorderedCacheTestSuite)

foreach(test_name IN LISTS test_lib_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC lib_test_utilities
           GTest::gmock
           GTest::gtest
           GTest::gtest_main
           Sourcetrail::lib
           $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
           $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)

  gtest_discover_tests(
    ${test_name}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
    TEST_PREFIX "unittests.")
endforeach()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(CommandlineHelperTestSuite PRIVATE -Wno-write-strings)
endif()
