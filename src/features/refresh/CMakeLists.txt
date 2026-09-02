# ${PROJECT_SOURCE_DIR}/src/features/refresh/CMakeLists.txt
#
# The refresh feature: the affordance that asks for a re-index. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# MessageRefresh moves here; MessageRefreshUI and MessageRefreshUIState deliberately do not. Despite
# the shared prefix they are unrelated: MessageRefresh asks the project to re-index, the other two
# ask the widgets to repaint after a zoom or a colour-scheme change. The repaint pair is app-wide
# and stays in the shared bus.
#
# The refresh *logic* is not here either -- RefreshInfo and RefreshInfoGenerator belong to the
# project feature, which owns the state machine this button pokes.

target_sources(
  Sourcetrail_lib
  PRIVATE logic/RefreshController.cpp
          logic/RefreshController.h
          logic/RefreshView.cpp
          logic/RefreshView.h
          messages/MessageRefresh.h)

target_sources(Sourcetrail_lib_gui PRIVATE ui/QtRefreshView.cpp ui/QtRefreshView.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME RefreshControllerTestSuite
    SOURCES tests/RefreshControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(RefreshControllerTestSuite PUBLIC ST_TESTING=1)
endif()
