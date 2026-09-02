# ${PROJECT_SOURCE_DIR}/src/features/code/CMakeLists.txt
#
# The code feature: the snippet/file view and everything that scrolls, focuses and annotates it. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# CodeScrollParams and CodeSnippetParams move into logic/ from component/view/helper/. They are the
# value types the controller hands the view, named across the message headers too, so they are the
# feature's own vocabulary rather than shared framework helpers.
#
# The SourceLocation family under data/location/ deliberately stays shared: the graph, the errors
# and the storage layer all read it, so it is not this feature's to own.

target_sources(
  Sourcetrail_lib
  PRIVATE logic/CodeController.cpp
          logic/CodeController.h
          logic/CodeScrollParams.h
          logic/CodeSnippetParams.cpp
          logic/CodeSnippetParams.h
          logic/CodeView.cpp
          logic/CodeView.h
          messages/MessageActivateFile.h
          messages/MessageActivateLocalSymbols.h
          messages/MessageActivateSourceLocations.h
          messages/MessageActivateTokenIds.h
          messages/MessageChangeFileView.h
          messages/MessageCodeReference.h
          messages/MessageCodeShowDefinition.h
          messages/MessageScrollCode.h
          messages/MessageScrollToLine.h
          messages/MessageShowReference.h
          messages/MessageShowScope.h
          messages/MessageToNextCodeReference.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/CodeFocusHandler.cpp
          ui/CodeFocusHandler.h
          ui/QtCodeArea.cpp
          ui/QtCodeArea.h
          ui/QtCodeField.cpp
          ui/QtCodeField.h
          ui/QtCodeFile.cpp
          ui/QtCodeFile.h
          ui/QtCodeFileList.cpp
          ui/QtCodeFileList.h
          ui/QtCodeFileSingle.cpp
          ui/QtCodeFileSingle.h
          ui/QtCodeFileTitleBar.cpp
          ui/QtCodeFileTitleBar.h
          ui/QtCodeFileTitleButton.cpp
          ui/QtCodeFileTitleButton.h
          ui/QtCodeNavigable.cpp
          ui/QtCodeNavigable.h
          ui/QtCodeNavigator.cpp
          ui/QtCodeNavigator.h
          ui/QtCodeSnippet.cpp
          ui/QtCodeSnippet.h
          ui/QtCodeView.cpp
          ui/QtCodeView.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME CodeControllerTestSuite
    SOURCES tests/CodeControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(CodeControllerTestSuite PUBLIC ST_TESTING=1)
endif()
