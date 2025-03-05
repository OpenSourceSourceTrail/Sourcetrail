# ${CMAKE_SOURCE_DIR}/tests/integration/lib_cxx/CMakeLists.txt
set(test_cxx_names CxxIncludeProcessingTestSuite CxxParserTestSuite CxxTypeNameTestSuite)

foreach(test_name IN LISTS test_cxx_names)
  add_sourcetrail_test(
    NAME
    ${test_name}
    SOURCES
    ${test_name}.cpp
    DEPS
    Sourcetrail::lib
    Sourcetrail::core::utility::TextAccess
    Sourcetrail::helper::TestFileRegister
    Sourcetrail::helper::TestStorage
    Sourcetrail::lib_cxx
    TEST_PREFIX
    "integration.lib_cxx."
    WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}/test/")
endforeach()
