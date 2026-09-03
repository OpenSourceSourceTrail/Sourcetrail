# ${PROJECT_SOURCE_DIR}/src/features/activation/CMakeLists.txt
#
# The activation feature: what happens when a symbol is activated -- the fan-out every other
# feature listens to. See src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# No ui/ tier: activation has a controller but no view. It turns one activation into the messages
# the code, graph and search features render, and owns no widget of its own.

target_sources(
  Sourcetrail_lib
  PRIVATE logic/ActivationController.cpp
          logic/ActivationController.h
          logic/ActivationListener.cpp
          logic/ActivationListener.h
          messages/MessageActivateBase.h
          messages/MessageActivateErrors.h
          messages/MessageActivateFullTextSearch.h
          messages/MessageActivateLegend.h
          messages/MessageActivateOverview.h
          messages/MessageActivateTokens.h
          messages/MessageActivateTrail.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME ActivationControllerTestSuite
    SOURCES tests/ActivationControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(ActivationControllerTestSuite PUBLIC ST_TESTING=1)
endif()
