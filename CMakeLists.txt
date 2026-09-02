cmake_minimum_required(VERSION 3.23)
include(cmake/version.cmake)

project(
  Sourcetrail
  DESCRIPTION "interactive source explorer"
  VERSION ${VERSION_STRING}
  HOMEPAGE_URL "opensourcesourcetrail.github.io"
  LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
#---------------------------------------------------------------------------------------------------------------------------------
option(BUILD_CXX_LANGUAGE_PACKAGE "Add C and C++ support to the Sourcetrail indexer." OFF)
option(BUILD_DOC "Build documentation." OFF)

option(ENABLE_UNIT_TEST "Build unit-tests." OFF)
option(ENABLE_GUI_TEST "Build gui-tests." OFF)
option(ENABLE_INTEGRATION_TEST "Build integration-tests." OFF)
option(ENABLE_ENGINE_HTTP_TEST "Build engine_http." OFF)
option(ENABLE_MCP_TEST "Build mcp." OFF)
option(ENABLE_COVERAGE "Build coverage report, run `ninja coverage`." OFF)
option(ENABLE_BUILD_WITH_TIME_TRACE "Trace building time." OFF)

option(SOURCETRAIL_CMAKE_VERBOSE "CMake verbose" OFF)
option(SOURCETRAIL_WARNING_AS_ERROR "Treat warnings as errors" OFF)
option(SOURCETRAIL_USE_LIBCPP "Use libc++ instead of libstdc++" OFF)

# The Java indexer needs Maven; default it on only when mvn is actually on PATH.
find_program(SR_MAVEN_EXECUTABLE mvn)
if(SR_MAVEN_EXECUTABLE)
  set(_sr_java_default ON)
else()
  set(_sr_java_default OFF)
endif()
option(BUILD_JAVA_INDEXER "Add the Java indexer plugin (requires Maven + JDK 21)." ${_sr_java_default})

set(SR_SAN
    ""
    CACHE STRING "Comma-separated sanitizers to build all code with: address, undefined, thread, memory.")
set(GCOV_VERSION
    "gcov-12"
    CACHE STRING "Version of gcov")
set(USE_ALTERNATE_LINKER
    ""
    CACHE STRING "Use alternate linker. Leave empty for system default; alternatives are 'gold', 'lld', 'bfd', 'mold'")
#---------------------------------------------------------------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
#---------------------------------------------------------------------------------------------------------------------------------
include(licenses)
include(create_symlink)
include(add_sourcetrail_library)
include(add_sourcetrail_interface)
include(add_sourcetrail_test)
include(assert_tier_deps)
include(clang-tidy)
include(cppcheck)
include(coverage)
#---------------------------------------------------------------------------------------------------------------------------------
include(sanitizers)
#---------------------------------------------------------------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  add_compile_options(-fcolor-diagnostics)
  if(SOURCETRAIL_USE_LIBCPP)
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # /Z7 embeds debug info in the .obj instead of routing every compile through mspdbsrv, which avoids
  # "C1041: cannot open program database" when ninja runs many cl.exe in parallel. Linker PDBs are unaffected.
  add_compile_options(/DNOMINMAX /Z7)
  add_link_options(/DEBUG /OPT:REF /OPT:ICF)
endif()

#---------------------------------------------------------------------------------------------------------------------------------
include(set_alternate_linker)
if(NOT "${USE_ALTERNATE_LINKER}" STREQUAL "")
  set_alternate_linker(${USE_ALTERNATE_LINKER})
endif()
#---------------------------------------------------------------------------------------------------------------------------------
include(compiler_warnings)
#---------------------------------------------------------------------------------------------------------------------------------
if(BUILD_DOC)
  include(doxygen)
endif()
# Variables ----------------------------------------------------------------------------------------------------------------------
if(WIN32)
  set(PLATFORM_INCLUDE "platform_includes/includesWindows.h")
elseif(APPLE)
  set(PLATFORM_INCLUDE "platform_includes/includesMac.h")
else()
  set(PLATFORM_INCLUDE "platform_includes/includesLinux.h")
endif()

set(BASH "")
if(WIN32)
  execute_process(COMMAND CMD /c where bash OUTPUT_VARIABLE BASH)
  string(REGEX REPLACE "\n$" "" BASH "${BASH}")
  if(SOURCETRAIL_CMAKE_VERBOSE)
    message("bash: '${BASH}'")
  endif()
endif()
#RPATH
if(UNIX AND NOT APPLE)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/lib/:$$ORIGIN/lib/")
endif()
# Settings -----------------------------------------------------------------------------------------------------------------------
find_package(expected-lite REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(SQLite3 CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
# Boost --------------------------------------------------------------------------------------------------------------------------
set(Boost_USE_MULTITHREAD ON)
set(Boost_USE_STATIC_LIBS
    ON
    CACHE BOOL "Set to ON to force the use of the static libraries.")
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(
  Boost 1.83
  COMPONENTS date_time filesystem locale program_options system
  REQUIRED)
# Qt -----------------------------------------------------------------------------------------------------------------------------
find_package(Qt6 6.10 REQUIRED COMPONENTS Widgets PrintSupport Network Svg Concurrent)
# Setup --------------------------------------------------------------------------------------------------------------------------
if(WIN32)
  include(copy_qt_binaries)
endif()

if(ENABLE_UNIT_TEST OR ENABLE_INTEGRATION_TEST)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
  add_library(gtest_main ./tests/gtest_main.cpp)
  add_library(Sourcetrail::gtest_main ALIAS gtest_main)
  target_link_libraries(
    gtest_main
    PUBLIC GTest::gmock GTest::gtest spdlog::spdlog Sourcetrail::lib::core::utility::utilityString
           Sourcetrail::lib::core::utility::TextCodec
    PRIVATE Sourcetrail::warnings)
endif()

string(LENGTH "${PROJECT_SOURCE_DIR}/" SOURCE_PATH_PREFIX_LEN)

# Libraries and executables; see src/CMakeLists.txt.
add_subdirectory(src)
add_subdirectory(indexers)

# The apps spawn the indexer worker out of build/app, so it has to be built alongside them. Declared
# here rather than next to each app: if(TARGET) is answered when the line is read, and the indexer
# plugins are configured after src/.
if(TARGET Sourcetrail_indexer)
  foreach(_app Sourcetrail Sourcetrail_cli)
    add_dependencies(${_app} Sourcetrail_indexer)
  endforeach()
endif()
# Language-package isolation -------------------------------------------------------------------------------------------------------
# Only the indexer worker may link a language package. If one creeps back into the GUI, the engine or the
# shared libraries, they stop compiling identically with and without BUILD_CXX_LANGUAGE_PACKAGE and the
# gRPC enums start depending on the build flag again. Fail the configure instead of finding out at runtime.
foreach(_target Sourcetrail_lib Sourcetrail_lib_engine Sourcetrail_lib_gui Sourcetrail Sourcetrail_engine
                Sourcetrail_cli)
  if(TARGET ${_target})
    get_target_property(_libs ${_target} LINK_LIBRARIES)
    if(_libs MATCHES "lib_cxx")
      message(FATAL_ERROR "${_target} must not link a language package; only Sourcetrail_indexer may.")
    endif()
  endif()
endforeach()
# Tests --------------------------------------------------------------------------------------------------------------------------
if(ENABLE_UNIT_TEST
   OR ENABLE_GUI_TEST
   OR ENABLE_INTEGRATION_TEST)
  if(WIN32 AND NOT ${SR_IS_CI})
    copy_qt_binaries("${PROJECT_BINARY_DIR}/test" FALSE)
  endif()
  file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/test")
  create_symlink("${PROJECT_SOURCE_DIR}/bin/test/data" "${PROJECT_BINARY_DIR}/test/data")

  find_package(
    Qt6 6.10
    COMPONENTS Test Sql
    REQUIRED)

  if(ENABLE_UNIT_TEST)
    add_subdirectory(${PROJECT_SOURCE_DIR}/src/lib/lib/tests/)
    add_subdirectory(${PROJECT_SOURCE_DIR}/src/lib/lib_gui/tests/)
    if(BUILD_CXX_LANGUAGE_PACKAGE)
      add_subdirectory(${PROJECT_SOURCE_DIR}/indexers/cxx/lib/tests/)
    endif()
  endif()

  if(ENABLE_INTEGRATION_TEST)
    add_subdirectory(tests)
    if(ENABLE_MCP_TEST)
      # The MCP server is not built, only tested: it is a Python client of the engine's HTTP API.
      add_subdirectory(mcp)
    endif()
  endif()
endif()
# Assets -------------------------------------------------------------------------------------------------------------------------
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/app")
create_symlink("${PROJECT_SOURCE_DIR}/bin/app/data" "${PROJECT_BINARY_DIR}/app/data")
create_symlink("${PROJECT_SOURCE_DIR}/bin/app/user" "${PROJECT_BINARY_DIR}/app/user")
# Install Executables ------------------------------------------------------------------------------------------------------------
set(APPIMAGE_ROOT "usr")
set(INSTALL_RUNTIME_DIR "${APPIMAGE_ROOT}/bin")

install(TARGETS Sourcetrail Sourcetrail_cli Sourcetrail_engine RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR})

# The indexer worker only exists when a language package was built (see indexers/cxx/).
if(TARGET Sourcetrail_indexer)
  install(TARGETS Sourcetrail_indexer RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR})
endif()

# gui/ and fonts/ are compiled into the binary via bin/app/resource.qrc, so they are not installed.
install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/bin/app/data/
  DESTINATION ${INSTALL_RUNTIME_DIR}/data/
  PATTERN "gui" EXCLUDE
  PATTERN "fonts" EXCLUDE)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/bin/app/user/ DESTINATION ${INSTALL_RUNTIME_DIR}/user/)

# Indexer plugin manifests. Without these the installed build discovers no indexer at all and comes up
# read-only, so they ship next to the binaries the manifests point at.
install(
  DIRECTORY ${PROJECT_BINARY_DIR}/app/plugins/
  DESTINATION ${INSTALL_RUNTIME_DIR}/plugins/
  OPTIONAL)

# Install desktop file for application
install(FILES "${PROJECT_SOURCE_DIR}/cmake/Sourcetrail.desktop" DESTINATION "${APPIMAGE_ROOT}/share/applications")

# Install application icon for desktop
install(FILES "${PROJECT_SOURCE_DIR}/bin/app/data/gui/installer/Sourcetrail.png"
        DESTINATION "${APPIMAGE_ROOT}/share/icons/hicolor/256x256/apps")
# Packaging ----------------------------------------------------------------------------------------------------------------------
# TODO(SOUR-104)
