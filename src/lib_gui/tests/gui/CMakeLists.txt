# ${CMAKE_SOURCE_DIR}/src/lib_gui/tests/gui/CMakeLists.txt
set(test_lib_gui_names
    QtContextMenuTestSuite
    QtNetworkFactoryTestSuite
    #QtScreenSearchBoxTestSuite
    QtSelfRefreshIconButtonTestSuite
    QtStartScreenTestSuite
    #RecentItemModelTestSuite NOTE: Not working in windows
)

foreach(test_name IN LISTS test_lib_gui_names)
  add_executable(${test_name} ${test_name}.cpp)

  target_link_libraries(
    ${test_name}
    PUBLIC Qt5::Test
           GTest::gmock
           Sourcetrail::lib
           Sourcetrail::lib_gui
           $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
           $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
           $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

  set_target_properties(
    ${test_name}
    PROPERTIES INCLUDE_CURRENT_DIR ON
               AUTOMOC ON
               RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

  add_test(
    NAME ${test_name}
    COMMAND ${test_name} "-platform" "offscreen"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/")
endforeach()
