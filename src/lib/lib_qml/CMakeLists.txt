# The QML front end. Unlike the widget GUI it replaced, this links lib_engine: the engine is a
# library here, not a separate process, so this target owns the SQLite index. Only indexing stays
# out of process, in the workers TaskBuildIndex spawns.
qt_add_library(Sourcetrail_lib_qml STATIC)
add_library(Sourcetrail::lib::lib_qml ALIAS Sourcetrail_lib_qml)

# Auto-detection of `pragma Singleton` is unreliable across configure runs; declaring it here is
# what actually gets `singleton Theme` into the generated qmldir. Without it every Theme.* binding
# silently evaluates to undefined at runtime rather than failing to build.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(
  Sourcetrail_lib_qml
  URI
  Sourcetrail
  VERSION
  1.0
  SOURCES
  AppShell.cpp
  AppShell.h
  Fonts.cpp
  Fonts.h
  GuiThread.h
  QmlDialogView.cpp
  QmlDialogView.h
  QmlGraphViewStyleImpl.cpp
  QmlGraphViewStyleImpl.h
  ShellMessages.cpp
  ShellMessages.h
  code/CodeHighlighter.cpp
  code/CodeHighlighter.h
  graph/EdgeRouter.cpp
  graph/EdgeRouter.h
  graph/GraphModels.cpp
  graph/GraphModels.h
  graph/GraphSnapshot.h
  graph/GraphViewModel.cpp
  graph/GraphViewModel.h
  graph/QmlGraphView.cpp
  graph/QmlGraphView.h
  network/QtIDECommunicationController.cpp
  network/QtIDECommunicationController.h
  network/QtNetworkFactory.cpp
  network/QtNetworkFactory.h
  network/QtTcpWrapper.cpp
  network/QtTcpWrapper.h
  search/ActionModel.cpp
  search/ActionModel.h
  search/MatchModel.cpp
  search/MatchModel.h
  search/QmlSearchView.cpp
  search/QmlSearchView.h
  search/SearchSnapshot.h
  search/SearchViewModel.cpp
  search/SearchViewModel.h
  shell/HistoryModel.cpp
  shell/HistoryModel.h
  shell/NavigationViewModel.cpp
  shell/NavigationViewModel.h
  shell/QmlStatusBarView.cpp
  shell/QmlStatusBarView.h
  shell/QmlStatusView.cpp
  shell/QmlStatusView.h
  shell/QmlUndoRedoView.cpp
  shell/QmlUndoRedoView.h
  shell/QmlViewLayout.cpp
  shell/QmlViewLayout.h
  shell/ShellSnapshot.h
  shell/StatusViewModel.cpp
  shell/StatusViewModel.h
  utility/globalStrings.h
  QML_FILES
  qml/GraphPanel.qml
  qml/Main.qml
  qml/Theme.qml
  qml/controls/StButton.qml
  qml/controls/StChip.qml
  qml/controls/StCornerMarks.qml
  qml/controls/StIconButton.qml
  qml/controls/StPanel.qml
  qml/controls/StSectionLabel.qml
  qml/controls/StSegmented.qml
  qml/controls/StTextField.qml
  qml/controls/StToggle.qml
  qml/search/CommandPalette.qml
  qml/search/MatchRow.qml
  qml/shell/AppFrame.qml
  qml/shell/ContextPanel.qml
  qml/shell/HistoryPage.qml
  qml/shell/IconRail.qml
  qml/shell/MenuBar.qml
  qml/shell/OverviewPage.qml
  qml/shell/PlaceholderPage.qml
  qml/shell/StatusBar.qml
  qml/shell/Toolbar.qml)

# The generated QML type registration includes every QML_ELEMENT header by basename
# (#include <GraphModels.h>), so each directory holding one has to be on the include path.
target_include_directories(Sourcetrail_lib_qml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/graph
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/shell
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/search
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/code)

target_link_libraries(
  Sourcetrail_lib_qml
  PUBLIC Sourcetrail::lib::lib
         Sourcetrail::lib::lib_engine
         Qt6::Quick
         Qt6::QuickControls2
         Qt6::Network
  PRIVATE Sourcetrail::lib::core
          Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::utility
          Sourcetrail::lib::core::utility::utilityApp
          Sourcetrail::lib::core::utility::file::FilePath
          Sourcetrail::lib::core::utility::file::FileSystem
          Sourcetrail::lib::core::utility::TextAccess
          Sourcetrail::lib::messaging
          Sourcetrail::warnings)
