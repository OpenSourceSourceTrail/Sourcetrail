# ${PROJECT_SOURCE_DIR}/src/features/status/CMakeLists.txt
#
# The status feature: the status bar and the status log window. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# Two controller/view pairs in one feature: StatusBarController drives the one-line bar,
# StatusController the log window behind it. They read the same MessageStatus traffic, so splitting
# them would put one message class in two features.
#
# Status.h itself stays in core -- it is a value type with no feature attached, used by the CLI and
# the indexer as well.

target_sources(
  Sourcetrail_lib
  PRIVATE logic/StatusBarController.cpp
          logic/StatusBarController.h
          logic/StatusBarView.cpp
          logic/StatusBarView.h
          logic/StatusController.cpp
          logic/StatusController.h
          logic/StatusView.cpp
          logic/StatusView.h
          messages/MessageClearStatusView.h
          messages/MessageShowStatus.h
          messages/MessageStatus.cpp
          messages/MessageStatus.h
          messages/MessageStatusFilterChanged.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtStatusBar.cpp
          ui/QtStatusBar.h
          ui/QtStatusBarView.cpp
          ui/QtStatusBarView.h
          ui/QtStatusView.cpp
          ui/QtStatusView.h)

if(ENABLE_UNIT_TEST)
  foreach(test_name IN ITEMS StatusBarControllerTestSuite StatusControllerTestSuite)
    add_sourcetrail_test(
      NAME ${test_name}
      SOURCES tests/${test_name}.cpp
      DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
           Sourcetrail::lib::proto::convert
      TEST_PREFIX "unittests.lib.")

    target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)
  endforeach()
endif()
