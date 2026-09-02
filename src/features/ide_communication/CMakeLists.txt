# ${PROJECT_SOURCE_DIR}/src/features/ide_communication/CMakeLists.txt
#
# The IDE-communication feature: the TCP channel Sourcetrail's editor plugins speak over. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# NetworkFactory and its Qt implementation come along: the factory exists only to build this
# feature's channel, so it is feature code that happened to sit in component/.
#
# QtNetworkFactoryTestSuite is a Qt Test, not a GTest, so it keeps the raw add_executable shape from
# lib_gui/tests/gui/CMakeLists.txt rather than going through add_sourcetrail_test(). It also used to
# reach into the Logic test tree with #include "../../../lib/tests/mocks/..."; it now links
# lib::mocks and spells the header plainly.

target_sources(
  Sourcetrail_lib
  PRIVATE logic/IDECommunicationController.cpp
          logic/IDECommunicationController.h
          logic/NetworkFactory.cpp
          logic/NetworkFactory.h
          logic/NetworkProtocolHelper.cpp
          logic/NetworkProtocolHelper.h
          messages/MessageIDECreateCDB.h
          messages/MessageMoveIDECursor.h
          messages/MessagePingReceived.h
          messages/MessagePluginPortChange.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtIDECommunicationController.cpp
          ui/QtIDECommunicationController.h
          ui/QtNetworkFactory.cpp
          ui/QtNetworkFactory.h
          ui/QtRequest.cpp
          ui/QtRequest.h
          ui/QtTcpWrapper.cpp
          ui/QtTcpWrapper.h)

if(ENABLE_UNIT_TEST)
  foreach(test_name IN ITEMS IDECommunicationControllerTestSuite NetworkProtocolHelperTestSuite)
    add_sourcetrail_test(
      NAME ${test_name}
      SOURCES tests/${test_name}.cpp
      DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess
           Sourcetrail::lib::proto::convert
      TEST_PREFIX "unittests.lib.")

    target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)
  endforeach()
endif()

if(ENABLE_GUI_TEST)
  add_executable(QtNetworkFactoryTestSuite tests/QtNetworkFactoryTestSuite.cpp)

  target_link_libraries(
    QtNetworkFactoryTestSuite
    PUBLIC Qt6::Test GTest::gmock lib::mocks Sourcetrail::lib::lib Sourcetrail::lib::lib_gui Sourcetrail::lib::proto::convert
    PRIVATE Sourcetrail::warnings)

  set_target_properties(
    QtNetworkFactoryTestSuite
    PROPERTIES INCLUDE_CURRENT_DIR ON
               AUTOMOC ON
               RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/test/")

  add_test(NAME QtNetworkFactoryTestSuite COMMAND QtNetworkFactoryTestSuite "-platform" "offscreen")
endif()
