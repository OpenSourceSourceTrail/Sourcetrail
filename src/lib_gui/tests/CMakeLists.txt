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

if(ENABLE_GUI_TEST)
  add_subdirectory(gui)
endif()
