# ${PROJECT_SOURCE_DIR}/src/lib/lib_qml/tests/CMakeLists.txt
add_sourcetrail_test(
  NAME globalStringsTestSuite
  SOURCES globalStringsTestSuite.cpp
  DEPS Sourcetrail::lib::lib_qml
  TEST_PREFIX "unittests.lib_qml.")

add_sourcetrail_test(
  NAME GraphSnapshotTestSuite
  SOURCES GraphSnapshotTestSuite.cpp
  DEPS Sourcetrail::lib::lib_qml
  TEST_PREFIX "unittests.lib_qml.")
