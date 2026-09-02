# ${PROJECT_SOURCE_DIR}/src/features/tooltip/CMakeLists.txt
#
# The tooltip feature. See src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# domain/ here is load-bearing well outside the feature: StorageAccess, ConvertQuery and the engine
# HTTP service all name TooltipInfo. That is the point of the domain tier -- it is the shared
# vocabulary, so it lives in Sourcetrail_lib and anyone may include it.

target_sources(
  Sourcetrail_lib
  PRIVATE domain/TooltipInfo.h
          domain/TooltipOrigin.h
          logic/TooltipController.cpp
          logic/TooltipController.h
          logic/TooltipView.cpp
          logic/TooltipView.h
          messages/MessageTooltipHide.h
          messages/MessageTooltipShow.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtTooltip.cpp
          ui/QtTooltip.h
          ui/QtTooltipView.cpp
          ui/QtTooltipView.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME TooltipControllerTestSuite
    SOURCES tests/TooltipControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(TooltipControllerTestSuite PUBLIC ST_TESTING=1)
endif()
