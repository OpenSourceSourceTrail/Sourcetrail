# ${CMAKE_SOURCE_DIR}/tests/integration/messaging/CMakeLists.txt
add_sourcetrail_test(
  NAME
  MessageQueueTestSuite
  SOURCES
  MessageQueueTestSuite.cpp
  DEPS
  Sourcetrail::messaging
  TEST_PREFIX
  "integration.messaging"
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")
