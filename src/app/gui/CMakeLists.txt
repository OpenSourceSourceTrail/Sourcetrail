#${CMAKE_SOURCE_DIR}/src/app/gui/CMakeLists.txt
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
  PUBLIC Sourcetrail::lib::core
         Sourcetrail::lib::lib
         Sourcetrail::lib::lib_gui
         Sourcetrail::lib::client
         Qt6::Widgets
         Qt6::Network
  PRIVATE Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::ScopedFunctor
          Sourcetrail::lib::core::utility::utilityApp
          Sourcetrail::lib::core::utility::commandline::CommandLineParser
          Sourcetrail::lib::proto::convert)

target_include_directories(Sourcetrail PRIVATE "${CMAKE_BINARY_DIR}/src/lib/lib_gui" "${CMAKE_BINARY_DIR}/src/lib")

set_target_properties(Sourcetrail PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app/")
# add platform specific libraries
if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail ${CORE_FOUNDATION})
endif()

add_dependencies(Sourcetrail Sourcetrail_indexer)

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/bin/app/data/gui/icon/sourcetrail.ico)
  message(FATAL "\"sourcetrail.ico\" not found. You may run `git submodule`")
endif()
file(COPY ${CMAKE_SOURCE_DIR}/bin/app/data/gui/icon/sourcetrail.ico DESTINATION ${CMAKE_BINARY_DIR})

if(WIN32 AND NOT ${SR_IS_CI})
  copy_qt_binaries("${CMAKE_BINARY_DIR}/app/" TRUE)
endif()

# The regression guard for the whole GUI <-> engine boundary: the GUI must reach the index database
# only through gRPC. A single #include of a storage header re-links SQLite into this binary and is
# otherwise invisible, so assert on the symbol table rather than on anyone's discipline.
if((ENABLE_UNIT_TEST OR ENABLE_INTEGRATION_TEST) AND NOT WIN32)
  find_program(NM_EXECUTABLE nm)
  if(NM_EXECUTABLE)
    add_test(NAME "guard.gui.linksNoSqlite"
             COMMAND ${CMAKE_COMMAND} -DBINARY=$<TARGET_FILE:Sourcetrail> -DNM=${NM_EXECUTABLE} -P
                     "${CMAKE_SOURCE_DIR}/cmake/assert_no_sqlite.cmake")
  endif()
endif()
