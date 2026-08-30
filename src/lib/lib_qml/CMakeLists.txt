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
  GuiThread.h
  QmlDialogView.cpp
  QmlDialogView.h
  QmlGraphViewStyleImpl.cpp
  QmlGraphViewStyleImpl.h
  ShellMessages.cpp
  ShellMessages.h
  network/QtIDECommunicationController.cpp
  network/QtIDECommunicationController.h
  network/QtNetworkFactory.cpp
  network/QtNetworkFactory.h
  network/QtTcpWrapper.cpp
  network/QtTcpWrapper.h
  utility/globalStrings.h
  QML_FILES
  qml/Main.qml
  qml/Theme.qml)

target_include_directories(Sourcetrail_lib_qml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
          Sourcetrail::lib::messaging
          Sourcetrail::warnings)
