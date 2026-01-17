# ${CMAKE_SOURCE_DIR}/src/lib_gui/tests/CMakeLists.txt
add_sourcetrail_test(
  NAME
  globalStringsTestSuite
  SOURCES
  globalStringsTestSuite.cpp
  DEPS
  Sourcetrail::lib_gui
  TEST_PREFIX
  "unittests.lib_gui."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  utilityAppTestSuite
  SOURCES
  utilityAppTestSuite.cpp
  DEPS
  Sourcetrail::libGui::utility::utilityApp
  TEST_PREFIX
  "unittests.lib_gui."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

# Check the following https://gist.github.com/seyhajin/77f5143b8be7d2120c3646b61765476d
if(MSVC AND MSVC_VERSION GREATER_EQUAL 1910)
  add_sourcetrail_test(
    NAME
    CxxVs17HeaderPathDetectorTestSuite
    SOURCES
    CxxVs17HeaderPathDetectorTestSuite.cpp
    DEPS
    Sourcetrail::lib_gui
    TEST_PREFIX
    "unittests.lib_gui."
    WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}/test/")
endif()

if(ENABLE_GUI_TEST)
  add_subdirectory(gui)
endif()
