# ${CMAKE_SOURCE_DIR}/src/scheduling/tests/CMakeLists.txt
set(test_lib_names TaskSchedulerTestSuite TaskManagerTestSuite)

add_sourcetrail_test(
  NAME
  TaskSchedulerTestSuite
  SOURCES
  TaskSchedulerTestSuite.cpp
  DEPS
  Sourcetrail::scheduling
  Sourcetrail::core::utility::ScopedFunctor
  TEST_PREFIX
  "unittests.scheduling."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")
add_sourcetrail_test(
  NAME
  TaskManagerTestSuite
  SOURCES
  TaskManagerTestSuite.cpp
  DEPS
  Sourcetrail::scheduling
  Sourcetrail::core::utility::ScopedFunctor
  TEST_PREFIX
  "unittests.scheduling."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")
