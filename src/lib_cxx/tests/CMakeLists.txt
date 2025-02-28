# ${CMAKE_SOURCE_DIR}/src/lib_cxx/tests/CMakeLists.txt
add_sourcetrail_test(
  NAME
  CompilationDatabaseTestSuite
  SOURCES
  CompilationDatabaseTestSuite.cpp
  DEPS
  Sourcetrail::lib
  Sourcetrail::lib_cxx
  Sourcetrail::core::utility::file::ScopedTemporaryFile
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  IncludeDirectiveTestSuite
  SOURCES
  IncludeDirectiveTestSuite.cpp
  DEPS
  Sourcetrail::lib
  Sourcetrail::lib_cxx
  Sourcetrail::core::utility::file::ScopedTemporaryFile
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")
