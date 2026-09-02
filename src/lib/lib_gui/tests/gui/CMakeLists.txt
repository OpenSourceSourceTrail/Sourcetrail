# ${PROJECT_SOURCE_DIR}/src/lib_gui/tests/gui/CMakeLists.txt
set(test_lib_gui_names
    QtContextMenuTestSuite
    #QtSelfRefreshIconButtonTestSuite NOTE: Not working in windows
    QtStartScreenTestSuite
    #RecentItemModelTestSuite NOTE: Not working in windows
)

foreach(test_name IN LISTS test_lib_gui_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    # lib::mocks rather than a #include "../../../lib/tests/mocks/..." reach across the tree.
    PUBLIC Qt6::Test GTest::gmock lib::mocks Sourcetrail::lib::lib Sourcetrail::lib::lib_gui
           Sourcetrail::lib::proto::convert
    PRIVATE Sourcetrail::warnings)

  set_target_properties(
    ${test_name}
    PROPERTIES INCLUDE_CURRENT_DIR ON
               AUTOMOC ON
               RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/test/")

  add_test(NAME ${test_name} COMMAND ${test_name} "-platform" "offscreen")
endforeach()
