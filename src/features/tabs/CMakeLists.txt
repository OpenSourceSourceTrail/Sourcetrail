# ${PROJECT_SOURCE_DIR}/src/features/tabs/CMakeLists.txt
#
# The tabs feature: the tab bar and the controller that opens, closes and selects tabs. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# Tab, TabbedView and QtTabbedView stay in the shared component framework. They are the container
# ComponentManager builds every feature's controller/view pair into -- generic machinery that the
# tab bar drives, not part of the tab bar itself. TabId stays in messaging for the same reason: it
# is how the bus routes, and 38 message headers name it.

target_sources(
  Sourcetrail_lib
  PRIVATE logic/TabsController.cpp
          logic/TabsController.h
          logic/TabsView.cpp
          logic/TabsView.h
          messages/MessageTabClose.h
          messages/MessageTabOpen.h
          messages/MessageTabOpenWith.h
          messages/MessageTabSelect.h
          messages/MessageTabState.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtTabBar.cpp
          ui/QtTabBar.h
          ui/QtTabsView.cpp
          ui/QtTabsView.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME TabsControllerTestSuite
    SOURCES tests/TabsControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(TabsControllerTestSuite PUBLIC ST_TESTING=1)
endif()
