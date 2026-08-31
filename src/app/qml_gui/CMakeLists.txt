# ${PROJECT_SOURCE_DIR}/src/app/qml_gui/CMakeLists.txt
#
# The whole application in one process: Qt Quick front end, business logic and the SQLite index.
# Only the indexer workers stay separate, spawned per job by TaskBuildIndex.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app/")

if(WIN32)
  file(
    WRITE ${PROJECT_BINARY_DIR}/Sourcetrail.rc
    "// Icon with lowest ID value placed first to ensure application icon\n" "// remains consistent on all systems.\n"
    "IDI_ICON1               ICON                    \"${PROJECT_BINARY_DIR}/Sourcetrail.ico\"\n")

  qt_add_executable(Sourcetrail main.cpp ${PROJECT_BINARY_DIR}/Sourcetrail.rc)

  target_compile_definitions(Sourcetrail PRIVATE NOMINMAX)

  set_target_properties(
    Sourcetrail
    PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE /DEBUG:FASTLINK"
               COMPILE_DEFINITIONS_DEBUG "_CONSOLE"
               LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
               COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE"
               LINK_FLAGS_RELEASE "/ENTRY:\"mainCRTStartup\" /SUBSYSTEM:CONSOLE /DEBUG"
               LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE"
               VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/app")

  target_compile_options(Sourcetrail PRIVATE $<$<CONFIG:Release>:/Zi>)
else()
  qt_add_executable(Sourcetrail main.cpp)
endif()

target_link_libraries(
  Sourcetrail
  PUBLIC Sourcetrail::lib::core
         Sourcetrail::lib::lib
         Sourcetrail::lib::lib_engine
         Sourcetrail::lib::lib_qml
         # The QML module is a static library, so its generated plugin -- which is what registers
         # the module and its types with the engine -- has to be linked explicitly. Without it the
         # scene loads to 'No module named "Sourcetrail" found'.
         Sourcetrail_lib_qmlplugin
         Qt6::Quick
  PRIVATE Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::ScopedFunctor
          Sourcetrail::lib::core::utility::utilityApp
          Sourcetrail::lib::core::utility::commandline::CommandLineParser
          # ApplicationSettingsPrefiller sniffs out the compiler and framework paths on first run.
          Sourcetrail::lib::utility::PathDetection
          Sourcetrail::warnings)

# bin/app/resource.qrc backs the ":/data/..." paths ResourcePaths hands out -- the fonts Theme.qml
# names and the graph node icons. Nothing had compiled it since the widget GUI was deleted, so every
# one of those lookups silently resolved to nothing: no vendored fonts, no node icons. It goes on
# the executable rather than the QML static library because a static library's resource initialiser
# is precisely the kind of object a linker drops when nothing references it.
# Note the variable form: qt_add_resources() given a target treats its next argument as a resource
# NAME, not a file, so passing the .qrc there compiles nothing and says nothing.
qt_add_resources(SOURCETRAIL_APP_RESOURCES ${PROJECT_SOURCE_DIR}/bin/app/resource.qrc)
target_sources(Sourcetrail PRIVATE ${SOURCETRAIL_APP_RESOURCES})

target_include_directories(Sourcetrail PRIVATE "${PROJECT_BINARY_DIR}/src/lib/lib")

set_target_properties(Sourcetrail PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app/")

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail ${CORE_FOUNDATION})
endif()

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/bin/app/data/gui/icon/sourcetrail.ico)
  message(FATAL_ERROR "\"sourcetrail.ico\" not found. You may run `git submodule`")
endif()
file(COPY ${PROJECT_SOURCE_DIR}/bin/app/data/gui/icon/sourcetrail.ico DESTINATION ${PROJECT_BINARY_DIR})

if(WIN32 AND NOT ${SR_IS_CI})
  copy_qt_binaries("${PROJECT_BINARY_DIR}/app/" TRUE)
endif()

# The QML front end must not grow a dependency on the widget module: a single QMessageBox would drag
# it back in, and it is invisible in review. Assert on the symbol table instead.
if((ENABLE_UNIT_TEST OR ENABLE_INTEGRATION_TEST) AND NOT WIN32)
  find_program(NM_EXECUTABLE nm)
  if(NM_EXECUTABLE)
    add_test(NAME "guard.gui.linksNoWidgets"
             COMMAND ${CMAKE_COMMAND} -DBINARY=$<TARGET_FILE:Sourcetrail> -DNM=${NM_EXECUTABLE} -P
                     "${PROJECT_SOURCE_DIR}/cmake/assert_no_widgets.cmake")
  endif()
endif()
