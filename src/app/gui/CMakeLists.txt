#${PROJECT_SOURCE_DIR}/src/app/gui/CMakeLists.txt
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app/")

if(WIN32)
  file(
    WRITE ${PROJECT_BINARY_DIR}/Sourcetrail.rc
    "// Icon with lowest ID value placed first to ensure application icon\n" "// remains consistent on all systems.\n"
    "IDI_ICON1               ICON                    \"${PROJECT_BINARY_DIR}/Sourcetrail.ico\"\n")

  add_executable(Sourcetrail main.cpp ${PROJECT_BINARY_DIR}/Sourcetrail.rc)

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
               VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/app")

  # generate pdb for release build
  target_compile_options(Sourcetrail PRIVATE $<$<CONFIG:Release>:/Zi>)
else()
  add_executable(Sourcetrail main.cpp)
endif()

target_link_libraries(
  Sourcetrail
  # An executable is nobody's dependency, so PUBLIC/PRIVATE make no difference here. PRIVATE says so.
  PRIVATE Sourcetrail::lib::core
          Sourcetrail::lib::lib
          Sourcetrail::lib::lib_gui
          Sourcetrail::lib::client
          Sourcetrail::app::engine_host
          Qt6::Widgets
          Qt6::Network
          Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::ScopedFunctor
          Sourcetrail::lib::core::utility::utilityApp
          Sourcetrail::lib::core::utility::commandline::CommandLineParser
          Sourcetrail::lib::proto::convert
          Sourcetrail::lib::utility::PathDetection)

target_include_directories(Sourcetrail PRIVATE "${PROJECT_BINARY_DIR}/src/lib/lib_gui" "${PROJECT_BINARY_DIR}/src/lib")

set_target_properties(Sourcetrail PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app/")
# add platform specific libraries
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

# Project warning set. PRIVATE so it applies here and is not inflicted on consumers.
target_link_libraries(Sourcetrail PRIVATE Sourcetrail::warnings)
