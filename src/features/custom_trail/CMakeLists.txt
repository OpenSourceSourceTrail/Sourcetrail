# ${PROJECT_SOURCE_DIR}/src/features/custom_trail/CMakeLists.txt
#
# The custom-trail feature: the dialog that builds a depth-graph query and the controller that
# answers it. See src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# No domain/ and no data/: the feature owns no types of its own and no storage. It queries through
# StorageAccess with the graph feature's vocabulary and hands the result to its view.

target_sources(
  Sourcetrail_lib
  PRIVATE logic/CustomTrailController.cpp
          logic/CustomTrailController.h
          logic/CustomTrailView.h
          messages/MessageCustomTrailShow.h)

target_sources(Sourcetrail_lib_gui PRIVATE ui/QtCustomTrailView.cpp ui/QtCustomTrailView.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME CustomTrailControllerTestSuite
    SOURCES tests/CustomTrailControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(CustomTrailControllerTestSuite PUBLIC ST_TESTING=1)
endif()
