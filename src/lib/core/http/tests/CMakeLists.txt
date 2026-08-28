# ${CMAKE_SOURCE_DIR}/src/lib/core/http/tests/CMakeLists.txt
add_sourcetrail_test(
  NAME HttpServerTestSuite
  SOURCES HttpServerTestSuite.cpp
  DEPS Sourcetrail::lib::core::http
  TEST_PREFIX "unittests.core."
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/")
