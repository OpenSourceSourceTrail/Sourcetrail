# ${PROJECT_SOURCE_DIR}/src/lib/client/tests/CMakeLists.txt
#
# EngineHttpService.cpp is compiled straight into the test rather than linked: the engine is an
# executable target, and the point of this suite is to drive the real server through the real wire
# from the real client. A test against a mocked transport could not catch the two halves disagreeing.
add_sourcetrail_test(
  NAME HttpStorageAccessTestSuite
  SOURCES HttpStorageAccessTestSuite.cpp EngineEventTestSuite.cpp
          "${PROJECT_SOURCE_DIR}/src/app/engine/EngineHttpService.cpp"
  DEPS lib::mocks
       Sourcetrail::lib::client
       Sourcetrail::lib::lib
       Sourcetrail::lib::proto::api
       Sourcetrail::lib::proto::convert
       Sourcetrail::lib::messaging
       Sourcetrail::lib::core
       Sourcetrail::lib::core::http
       Sourcetrail::lib::core::utility::logging
       Sourcetrail::lib::core::utility::TextAccess
  TEST_PREFIX "unittests.client.")

target_include_directories(HttpStorageAccessTestSuite PRIVATE "${PROJECT_SOURCE_DIR}/src/app/engine")
