# ${CMAKE_SOURCE_DIR}/src/lib/client/tests/CMakeLists.txt
#
# EngineServiceImpl.cpp is compiled straight into the test rather than linked: the engine is an
# executable target, and the point of this suite is to drive the real server through the real wire
# from the real client. A test against a mocked stub could not catch the two halves disagreeing.
add_sourcetrail_test(
  NAME
  GrpcStorageAccessTestSuite
  SOURCES
  GrpcStorageAccessTestSuite.cpp
  "${CMAKE_SOURCE_DIR}/src/app/engine/EngineServiceImpl.cpp"
  DEPS
  lib::mocks
  Sourcetrail::lib::client
  Sourcetrail::lib::lib
  Sourcetrail::lib::proto::api
  Sourcetrail::lib::proto::convert
  Sourcetrail::lib::messaging
  Sourcetrail::lib::core
  Sourcetrail::lib::core::utility::logging
  Sourcetrail::lib::core::utility::TextAccess
  Sourcetrail::gtest_main
  gRPC::grpc++
  TEST_PREFIX
  "unittests.client."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

target_include_directories(GrpcStorageAccessTestSuite PRIVATE "${CMAKE_SOURCE_DIR}/src/app/engine")
