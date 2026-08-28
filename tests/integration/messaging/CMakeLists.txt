# ${PROJECT_SOURCE_DIR}/tests/integration/messaging/CMakeLists.txt
add_sourcetrail_test(
  NAME MessageQueueTestSuite
  SOURCES MessageQueueTestSuite.cpp
  DEPS Sourcetrail::lib::messaging
  TEST_PREFIX "integration.messaging"
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/test/")
