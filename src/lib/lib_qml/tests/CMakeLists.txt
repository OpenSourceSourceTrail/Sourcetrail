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

add_sourcetrail_test(
  NAME CodeHighlighterTestSuite
  SOURCES CodeHighlighterTestSuite.cpp
  DEPS Sourcetrail::lib::lib_qml
  TEST_PREFIX "unittests.lib_qml.")

# The highlighting rules and the colour scheme are read from the shared data directory at runtime.
# Tests run from ${PROJECT_BINARY_DIR}/test/, so hand them the source location rather than a
# relative path that breaks the moment the working directory changes.
target_compile_definitions(CodeHighlighterTestSuite PRIVATE
                           SOURCETRAIL_SHARED_DATA_DIR="${PROJECT_SOURCE_DIR}/bin/app/")
