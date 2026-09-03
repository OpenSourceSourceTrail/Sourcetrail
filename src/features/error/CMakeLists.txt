# ${PROJECT_SOURCE_DIR}/src/features/error/CMakeLists.txt
#
# The error feature: indexing errors, their counts and filters, and the table that lists them. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# StorageError.h joins domain/ rather than data/: it is the POD row shape, named by Storage,
# SqliteIndexStorage and proto/Convert alike, so it has to stay in Sourcetrail_lib. Only code that
# talks to SQLite belongs in data/, and this feature has none of its own -- errors are written
# through the shared index storage.

target_sources(
  Sourcetrail_lib
  PRIVATE domain/ErrorCountInfo.cpp
          domain/ErrorCountInfo.h
          domain/ErrorFilter.h
          domain/ErrorInfo.h
          domain/StorageError.h
          logic/ErrorController.cpp
          logic/ErrorController.h
          logic/ErrorView.cpp
          logic/ErrorView.h
          messages/MessageErrorCountClear.h
          messages/MessageErrorCountUpdate.h
          messages/MessageErrorsAll.h
          messages/MessageErrorsForFile.h
          messages/MessageErrorsHelpMessage.h
          messages/MessageFilterErrorCountUpdate.h
          messages/MessageShowError.h)

target_sources(Sourcetrail_lib_gui PRIVATE ui/QtErrorView.cpp ui/QtErrorView.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME ErrorControllerTestSuite
    SOURCES tests/ErrorControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(ErrorControllerTestSuite PUBLIC ST_TESTING=1)
endif()
