# ${PROJECT_SOURCE_DIR}/src/lib/lib_gui/CMakeLists.txt
add_custom_target(versionnumber ALL)

add_library(Sourcetrail_lib_gui ${PROJECT_SOURCE_DIR}/bin/app/resource.qrc
                                ${PROJECT_BINARY_DIR}/src/lib/lib_gui/productVersion.h)
add_library(Sourcetrail::lib::lib_gui ALIAS Sourcetrail_lib_gui)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE utility/globalStrings.h
          platform_includes/includesLinux.h
          platform_includes/includesMac.h
          platform_includes/includesWindows.h
          qt/element/button/QtHelpButton.cpp
          qt/element/button/QtHelpButton.h
          qt/element/button/QtHoverButton.cpp
          qt/element/button/QtHoverButton.h
          qt/element/button/QtIconButton.cpp
          qt/element/button/QtIconButton.h
          qt/element/button/QtIconStateButton.cpp
          qt/element/button/QtIconStateButton.h
          qt/element/button/QtSelfRefreshIconButton.cpp
          qt/element/button/QtSelfRefreshIconButton.h
          qt/element/model/RecentItemModel.hpp
          qt/element/model/RecentItemModel.cpp
          qt/engine/QtEngineSupervisor.cpp
          qt/engine/QtEngineSupervisor.h
          qt/element/dialog/QtLineEdit.cpp
          qt/element/dialog/QtLineEdit.h
          qt/element/dialog/QtListBox.cpp
          qt/element/dialog/QtListBox.h
          qt/element/dialog/QtListBoxItem.cpp
          qt/element/dialog/QtListBoxItem.h
          qt/element/dialog/QtLocationPicker.cpp
          qt/element/dialog/QtLocationPicker.h
          qt/element/dialog/QtPathListBox.cpp
          qt/element/dialog/QtPathListBox.h
          qt/element/dialog/QtPathListBoxItem.cpp
          qt/element/dialog/QtPathListBoxItem.h
          qt/element/dialog/QtProgressBar.cpp
          qt/element/dialog/QtProgressBar.h
          qt/element/dialog/QtStringListBox.cpp
          qt/element/dialog/QtStringListBox.h
          qt/element/dialog/QtStringListBoxItem.cpp
          qt/element/dialog/QtStringListBoxItem.h
          qt/element/dialog/QtTextEdit.cpp
          qt/element/dialog/QtTextEdit.h
          qt/element/QtTable.cpp
          qt/element/QtTable.h
          qt/utility/compatibilityQt.cpp
          qt/utility/compatibilityQt.h
          qt/utility/QtContextMenu.cpp
          qt/utility/QtContextMenu.h
          qt/utility/QtDeviceScaledPixmap.cpp
          qt/utility/QtDeviceScaledPixmap.h
          qt/utility/QtFileDialog.cpp
          qt/utility/QtFileDialog.h
          qt/utility/QtFilesAndDirectoriesDialog.cpp
          qt/utility/QtFilesAndDirectoriesDialog.h
          qt/utility/QtFlowLayout.cpp
          qt/utility/QtFlowLayout.h
          qt/utility/QtHelpButtonInfo.cpp
          qt/utility/QtHelpButtonInfo.h
          qt/utility/QtHighlighter.cpp
          qt/utility/QtHighlighter.h
          qt/utility/QtScrollSpeedChangeListener.cpp
          qt/utility/QtScrollSpeedChangeListener.h
          qt/utility/QtThreadedFunctor.h
          qt/utility/utilityQt.cpp
          qt/utility/utilityQt.h
          qt/view/QtCompositeView.cpp
          qt/view/QtCompositeView.h
          qt/view/QtDialogView.cpp
          qt/view/QtDialogView.h
          qt/view/QtMainView.cpp
          qt/view/QtMainView.h
          qt/view/QtTabbedView.cpp
          qt/view/QtTabbedView.h
          qt/view/QtViewFactory.cpp
          qt/view/QtViewFactory.h
          qt/view/QtViewWidgetWrapper.cpp
          qt/view/QtViewWidgetWrapper.h
          qt/window/QtAbout.cpp
          qt/window/QtAbout.h
          qt/window/QtKeyboardShortcuts.cpp
          qt/window/QtKeyboardShortcuts.h
          qt/window/QtKnownProgressDialog.cpp
          qt/window/QtKnownProgressDialog.h
          qt/window/QtLicenseWindow.cpp
          qt/window/QtLicenseWindow.h
          qt/window/QtMainWindow.cpp
          qt/window/QtMainWindow.h
          qt/window/QtPathListDialog.cpp
          qt/window/QtPathListDialog.h
          qt/window/QtPreferencesWindow.cpp
          qt/window/QtPreferencesWindow.h
          qt/window/QtProgressBarDialog.cpp
          qt/window/QtProgressBarDialog.h
          qt/window/QtSelectPathsDialog.cpp
          qt/window/QtSelectPathsDialog.h
          qt/window/QtStartScreen.cpp
          qt/window/QtStartScreen.hpp
          qt/window/QtTextEditDialog.cpp
          qt/window/QtTextEditDialog.h
          qt/window/QtUnknownProgressDialog.cpp
          qt/window/QtUnknownProgressDialog.h
          qt/window/QtWindow.cpp
          qt/window/QtWindow.h
          qt/window/QtWindowBase.cpp
          qt/window/QtWindowBase.h
          qt/window/QtWindowStack.cpp
          qt/window/QtWindowStack.h
          qt/QtApplication.cpp
          qt/QtApplication.h
          qt/QtCoreApplication.cpp
          qt/QtCoreApplication.h)

target_link_libraries(
  Sourcetrail_lib_gui
  PUBLIC Sourcetrail::lib::lib
         Sourcetrail::lib::client # QtEngineSupervisor hands its EngineChannel to the GUI
         Qt6::Widgets
         Qt6::Network
         Qt6::Concurrent
         Qt6::Svg
         Sourcetrail::lib::core::utility::SingleValueCache
         Sourcetrail::lib::core::utility::LowMemoryStringMap
         Sourcetrail::lib::core::utility::Migrator
         Sourcetrail::lib::core::utility::file::FilePathFilter
         Sourcetrail::lib::core::utility::file::FileSystem
         Sourcetrail::lib::core::utility::utilityApp
  PRIVATE Sourcetrail::lib::core
          Sourcetrail::lib::core::utility::rangesTo
          Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::OrderedCache
          Sourcetrail::lib::core::utility::ScopedFunctor
          Sourcetrail::lib::core::utility::Status
          Sourcetrail::lib::core::utility::TextAccess
          Sourcetrail::lib::core::utility::TextCodec
          Sourcetrail::lib::core::utility::toUnderlying
          Sourcetrail::lib::core::utility::utility
          Sourcetrail::lib::core::utility::utilityUuid
          Sourcetrail::lib::core::utility::file::utilityFile
          Sourcetrail::lib::core::utility::file::FilePath
          Sourcetrail::lib::core::utility::file::FileManager
          Sourcetrail::lib::utility::PathDetection
          Sourcetrail::lib::core::utility::utilityApp
          Sourcetrail::lib::messaging
          Boost::locale)

add_custom_command(
  TARGET versionnumber
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${PROJECT_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/version.cmake
  BYPRODUCTS ${PROJECT_BINARY_DIR}/src/lib/lib_gui/productVersion.h
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMENT "check/update version number")

add_dependencies(Sourcetrail_lib_gui versionnumber)

# One include root, e.g. #include "qt/view/QtMainWindow.h". The binary dirs carry the generated
# includes.h / productVersion.h and language_packages.h.
target_include_directories(Sourcetrail_lib_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/src/lib/lib_gui
                                                      ${PROJECT_BINARY_DIR}/src/lib/lib)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/platform_includes/includes.h.in"
               "${PROJECT_BINARY_DIR}/src/lib/lib_gui/includes.h")

configure_file("${PROJECT_SOURCE_DIR}/cmake/productVersion.h.in"
               "${PROJECT_BINARY_DIR}/src/lib/lib_gui/productVersion.h")

set_target_properties(
  Sourcetrail_lib_gui
  PROPERTIES AUTOMOC ON
             AUTORCC ON
             INCLUDE_CURRENT_DIR ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set_source_files_properties(${PROJECT_BINARY_DIR}/Sourcetrail_lib_gui_autogen/mocs_compilation.cpp
                              PROPERTIES COMPILE_FLAGS "-Wno-useless-cast")
endif()

set_property(SOURCE ${PROJECT_BINARY_DIR}/src/lib/lib_gui/productVersion.h PROPERTY SKIP_AUTOMOC ON)

# Project warning set. PRIVATE so it applies here and is not inflicted on consumers.
target_link_libraries(Sourcetrail_lib_gui PRIVATE Sourcetrail::warnings)

# Tier guard. Presentation may name Logic; it must never name Data or SQLite. Asserted here, on
# the library, because the Sourcetrail binary hosts the engine in-process and so contains SQLite
# legitimately -- see cmake/assert_tier_deps.cmake.
assert_no_transitive_link(Sourcetrail_lib_gui Sourcetrail_lib_engine Sourcetrail::lib::lib_engine
                          SQLite::SQLite3 CppSQLite::CppSQLite3)

# The symbol-table half of the same guard: assert_no_transitive_link above checks what CMake was
# told, this checks what the compiler emitted. Linux/macOS only -- it needs nm.
if((ENABLE_UNIT_TEST OR ENABLE_INTEGRATION_TEST) AND NOT WIN32)
  find_program(NM_EXECUTABLE nm)
  if(NM_EXECUTABLE)
    add_test(NAME "guard.gui.linksNoSqlite"
             COMMAND ${CMAKE_COMMAND} -DBINARY=$<TARGET_FILE:Sourcetrail_lib_gui> -DNM=${NM_EXECUTABLE} -P
                     "${CMAKE_SOURCE_DIR}/cmake/assert_no_sqlite.cmake")
  endif()
endif()
