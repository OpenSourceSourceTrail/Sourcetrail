#${CMAKE_SOURCE_DIR}/src/app/CMakeLists.txt
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app/")

if(WIN32)
  file(
    WRITE ${CMAKE_BINARY_DIR}/Sourcetrail.rc
    "// Icon with lowest ID value placed first to ensure application icon\n" "// remains consistent on all systems.\n"
    "IDI_ICON1               ICON                    \"${CMAKE_BINARY_DIR}/Sourcetrail.ico\"\n")

  add_executable(Sourcetrail main.cpp ${CMAKE_BINARY_DIR}/Sourcetrail.rc)

  target_compile_definitions(Sourcetrail PRIVATE NOMINMAX)

  # also show the console when running a release build.
  set_target_properties(
    Sourcetrail
    PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE /DEBUG:FASTLINK"
               COMPILE_DEFINITIONS_DEBUG "_CONSOLE"
               LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
               COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE"
               LINK_FLAGS_RELEASE "/ENTRY:\"mainCRTStartup\" /SUBSYSTEM:CONSOLE /DEBUG"
               LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE"
               VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/app")

  # generate pdb for release build
  add_compile_options(Sourcetrail PRIVATE $<$<CONFIG:Release>:/Zi>)
else()
  add_executable(Sourcetrail main.cpp)
endif()

target_link_libraries(
  Sourcetrail
  PUBLIC Sourcetrail::core
         Sourcetrail::lib
         Sourcetrail::lib_gui
         $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
         Qt6::Widgets
         Qt6::Network
  PRIVATE Sourcetrail::core::utility::logging
          Sourcetrail::core::utility::ScopedFunctor
          Sourcetrail::libGui::utility::utilityApp
          # $<NOT:$<BOOL:OFF>>
          $<$<NOT:$<BOOL:${SOURCETRAIL_WASM}>>:Sourcetrail::core::utility::commandline::CommandLineParser>
          )

target_include_directories(
  Sourcetrail
  PRIVATE "${APP_INCLUDE_PATHS}"
          "${LIB_INCLUDE_PATHS}"
          "${LIB_GUI_INCLUDE_PATHS}"
          "${CMAKE_BINARY_DIR}/src/lib_gui"
          "${CMAKE_BINARY_DIR}/src/lib")

target_include_directories(Sourcetrail SYSTEM PRIVATE "${EXTERNAL_INCLUDE_PATHS}")
set_target_properties(Sourcetrail PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app/")
# add platform specific libraries
if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail ${CORE_FOUNDATION})
endif()

if(NOT SOURCETRAIL_WASM)
  add_dependencies(Sourcetrail Sourcetrail_indexer)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/bin/app/data/gui/icon/sourcetrail.ico)
  message(FATAL "\"sourcetrail.ico\" not found. You may run `git submodule`")
endif()
file(COPY ${CMAKE_SOURCE_DIR}/bin/app/data/gui/icon/sourcetrail.ico DESTINATION ${CMAKE_BINARY_DIR})

if(WIN32 AND NOT ${SR_IS_CI})
  copy_qt_binaries("${CMAKE_BINARY_DIR}/app/" TRUE)
endif()
