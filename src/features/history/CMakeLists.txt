# ${PROJECT_SOURCE_DIR}/src/features/history/CMakeLists.txt
#
# The history feature: the back/forward stack and the UI that browses it. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning behind it.
#
# The plan listed `history` and `undo_redo` as two features. They are one: UndoRedoController *is*
# the history stack, QtHistoryList is that stack rendered, and the MessageHistory* messages are the
# only traffic between them. Splitting them would put a controller in one feature and its own
# messages in another.

target_sources(
  Sourcetrail_lib
  PRIVATE logic/UndoRedoController.cpp
          logic/UndoRedoController.h
          logic/UndoRedoView.cpp
          logic/UndoRedoView.h
          messages/MessageHistoryRedo.h
          messages/MessageHistoryToPosition.h
          messages/MessageHistoryUndo.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtHistoryList.cpp
          ui/QtHistoryList.h
          ui/QtUndoRedo.cpp
          ui/QtUndoRedo.h
          ui/QtUndoRedoView.cpp
          ui/QtUndoRedoView.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME UndoRedoControllerTestSuite
    SOURCES tests/UndoRedoControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(UndoRedoControllerTestSuite PUBLIC ST_TESTING=1)
endif()
