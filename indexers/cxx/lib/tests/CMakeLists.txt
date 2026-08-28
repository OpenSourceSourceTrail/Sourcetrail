# ${PROJECT_SOURCE_DIR}/indexers/cxx/lib/tests/CMakeLists.txt
add_sourcetrail_test(
  NAME CompilationDatabaseTestSuite
  SOURCES CompilationDatabaseTestSuite.cpp
  DEPS Sourcetrail::lib::lib Sourcetrail::lib::lib_cxx Sourcetrail::lib::core::utility::file::ScopedTemporaryFile
  TEST_PREFIX "unittests.core."
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/test/")
