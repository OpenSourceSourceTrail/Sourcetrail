#${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
include(cmake/version.cmake)

project(Sourcetrail
        DESCRIPTION "interactive source explorer"
        VERSION     ${VERSION_STRING}
        HOMEPAGE_URL "opensourcesourcetrail.github.io"
        LANGUAGES C CXX)

set(BUILD_CXX_LANGUAGE_PACKAGE          OFF CACHE BOOL "Add C and C++ support to the Sourcetrail indexer.")
set(BUILD_DOC                           OFF CACHE BOOL "Build documentation.")
set(ENABLE_UNIT_TEST                    OFF CACHE BOOL "Build unit-tests.")
set(ENABLE_INTEGRATION_TEST             OFF CACHE BOOL "Build integration-tests.")
set(ENABLE_E2E_TEST                     OFF CACHE BOOL "Build e2e-tests.")
set(ENABLE_CLANG_TIDY                   OFF CACHE BOOL "Run clang-tidy after building.")
set(ENABLE_CPPCHECK                     OFF CACHE BOOL "Run cppcheck after building.")
set(ENABLE_SANITIZER_ADDRESS            OFF CACHE BOOL "Inject address sanitizer.")
set(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR OFF CACHE BOOL "Inject undefined behavior sanitizer.")
set(ENABLE_SANITIZER_THREAD             OFF CACHE BOOL "Inject thread sanitizer.")
set(ENABLE_SANITIZER_MEMORY             OFF CACHE BOOL "Inject memory sanitizer.")
set(ENABLE_COVERAGE                     OFF CACHE BOOL "Build coverage report run `ninja coverage`.")
set(ENABLE_BUILD_WITH_TIME_TRACE        OFF CACHE BOOL "Trace building time.")
set(SOURCETRAIL_CMAKE_VERBOSE           OFF CACHE BOOL "CMake verbose")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_STANDARD            17)
set(CMAKE_C_STANDARD              11)

include(cmake/add_files.cmake)
include(cmake/version_setup.cmake)
include(cmake/licenses.cmake)
include(cmake/create_symlink.cmake)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  add_compile_options(-fcolor-diagnostics)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
endif()

include(cmake/set_alternate_linker.cmake)

set(USE_ALTERNATE_LINKER "" CACHE STRING "Use alternate linker. Leave empty for system default; alternatives are 'gold', 'lld', 'bfd', 'mold'")
if(NOT "${USE_ALTERNATE_LINKER}" STREQUAL "")
  set_alternate_linker(${USE_ALTERNATE_LINKER})
endif()

# TODO(Hussein): Move to Conan V2
# Add project_options v0.26.3
# https://github.com/cpp-best-practices/project_options
include(FetchContent)
FetchContent_Declare(
  _project_options
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL "https://github.com/aminya/project_options/archive/refs/tags/v0.26.3.zip")
FetchContent_MakeAvailable(_project_options)
include(${_project_options_SOURCE_DIR}/Index.cmake)
# Variables --------------------------------------------------------------------
set(PROJECT_NAME Sourcetrail)

if(WIN32)
  set(PLATFORM_INCLUDE "includesWindows.h")
elseif(APPLE)
  set(PLATFORM_INCLUDE "includesMac.h")
else()
  set(PLATFORM_INCLUDE "includesLinux.h")
endif()

set(BASH "")
if(WIN32)
  execute_process(COMMAND CMD /c where bash OUTPUT_VARIABLE BASH)
  string(REGEX REPLACE "\n$" "" BASH "${BASH}")
  if(SOURCETRAIL_CMAKE_VERBOSE)
    message("bash: '${BASH}'")
  endif()
endif()
# Project ----------------------------------------------------------------------
include(${_project_options_SOURCE_DIR}/src/DynamicProjectOptions.cmake)

if(BUILD_DOC)
  include(cmake/doxygen.cmake)
endif()

if(ENABLE_COVERAGE)
  set(ENABLE_COVERAGE "ENABLE_COVERAGE")
  configure_file("${CMAKE_SOURCE_DIR}/cmake/coverage.sh.in"
                 "${CMAKE_BINARY_DIR}/coverage.sh")
  file(CHMOD "${CMAKE_BINARY_DIR}/coverage.sh"
       FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
  add_custom_target(Coverage
    COMMAND ${CMAKE_BINARY_DIR}/coverage.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()

project_options(
  ENABLE_CONAN
  # CONAN_OPTIONS    # Extra options to pass to conan
  # MSVC_WARNINGS    # Override the defaults for the MSVC warnings
  # CLANG_WARNINGS   # Override the defaults for the CLANG warnings
  # GCC_WARNINGS     # Override the defaults for the GCC warnings
  ${ENABLE_CPPCHECK}
  ${ENABLE_CLANG_TIDY}
  ${ENABLE_COVERAGE}
  ENABLE_VS_ANALYSIS
  # ENABLE_INTERPROCEDURAL_OPTIMIZATION
  # ENABLE_NATIVE_OPTIMIZATION
  ${ENABLE_DOXYGEN}
  ${ENABLE_SANITIZER_ADDRESS}
  ${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}
  ${ENABLE_SANITIZER_THREAD}
  ${ENABLE_SANITIZER_MEMORY}
  # ENABLE_PCH
  # PCH_HEADERS
  # WARNINGS_AS_ERRORS
  # ENABLE_INCLUDE_WHAT_YOU_USE
  # ENABLE_BUILD_WITH_TIME_TRACE
  # ENABLE_UNITY
)

#RPATH
if(UNIX AND NOT APPLE)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/lib/:$$ORIGIN/lib/")
endif()

# Settings ---------------------------------------------------------------------
find_package(SQLite3 CONFIG REQUIRED)
find_package(TinyXML CONFIG REQUIRED)
find_package(fmt     CONFIG REQUIRED)
# Boost ------------------------------------------------------------------------
set(Boost_USE_MULTITHREAD ON)
set(Boost_USE_STATIC_LIBS ON CACHE BOOL "Set to ON to force the use of the static libraries.")
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.67 COMPONENTS system program_options filesystem date_time locale REQUIRED)
# Qt ---------------------------------------------------------------------------
set(QT_MIN_VERSION     "5.12.0")
set(QT_MIN_VERSION_HEX 0x051200)
find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS Widgets PrintSupport Network Svg REQUIRED)

if(WIN32)
  find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS WinExtras REQUIRED)
endif()

if(Qt5Widgets_FOUND)
  if(SOURCETRAIL_CMAKE_VERBOSE)
    message(STATUS "Found Qt ${Qt5Widgets_VERSION_STRING}")
  endif()

  # FIX: Qt was built with -reduce-relocations
  if(Qt5_POSITION_INDEPENDENT_CODE)
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()

  # The following define makes your compiler emit warnings if you use
  # any feature of Qt which as been marked as deprecated (the exact warnings
  # depend on your compiler). Please consult the documentation of the
  # deprecated API in order to know how to port your code away from it.
  add_definitions(-DQT_DEPRECATED_WARNINGS)

  # You can also make your code fail to compile if you use deprecated APIs.
  # In order to do so, uncomment the following line.
  # You can also select to disable deprecated APIs only up to a certain version of Qt.
  #DEFINES += QT_DISABLE_DEPRECATED_BEFORE=${QT_MIN_VERSION_HEX}    # disables all the APIs deprecated at or before the specified Qt version
  add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=${QT_MIN_VERSION_HEX})
endif()
# Setup ------------------------------------------------------------------------
if(WIN32)
  include(cmake/copy_qt_binaries.cmake)
endif()

if(ENABLE_UNIT_TEST OR ENABLE_INTEGRATION_TEST OR ENABLE_E2E_TEST)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  find_package(Catch2 CONFIG REQUIRED)
  find_package(trompeloeil CONFIG REQUIRED)
  include(Catch)
  include(GoogleTest)
endif()

add_subdirectory(src/external)
add_subdirectory(src/lib)
add_subdirectory(src/lib_gui)

# Lib Utility ------------------------------------------------------------------
add_subdirectory(src/lib_utility)
# core -------------------------------------------------------------------------
add_subdirectory(src/core)
add_subdirectory(src/scheduling)
add_subdirectory(src/messaging)
# Lib Cxx ----------------------------------------------------------------------
if(BUILD_CXX_LANGUAGE_PACKAGE)
  add_subdirectory(src/lib_cxx)
endif()
# Lib --------------------------------------------------------------------------
add_library(Sourcetrail_lib ${LIB_FILES})
add_library(Sourcetrail::lib ALIAS Sourcetrail_lib)

target_include_directories(
  Sourcetrail_lib
  PUBLIC
    ${LIB_INCLUDE_PATHS}
    ${LIB_GUI_INCLUDE_PATHS}
    ${CMAKE_SOURCE_DIR}/src/lib_gui/utility/
    ${CMAKE_SOURCE_DIR}/src/lib_gui/utility/path_detector/
    ${CMAKE_BINARY_DIR}/src/lib)

target_link_libraries(
  Sourcetrail_lib
  PUBLIC
    project_options
    project_warnings
    Sourcetrail::messaging
    Sourcetrail::scheduling
    CppSQLite::CppSQLite3
  PRIVATE
    Sourcetrail::core
    Sourcetrail::lib_utility
    Sourcetrail::lib_gui
    $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
    Boost::date_time
    Boost::filesystem
    Boost::program_options
    Boost::system
    SQLite::SQLite3
    TinyXML::TinyXML
    fmt::fmt
    $<$<PLATFORM_ID:Windows>:bcrypt>)

#configure language package defines
configure_file("${CMAKE_SOURCE_DIR}/cmake/language_packages.h.in"
               "${CMAKE_BINARY_DIR}/src/lib/language_packages.h")
# Lib gui_utility --------------------------------------------------------------
add_library(Sourcetrail_lib_gui_utility)
add_library(Sourcetrail::lib_gui::utility ALIAS Sourcetrail_lib_gui_utility)

target_sources(
  Sourcetrail_lib_gui_utility
  PRIVATE
    src/lib_gui/utility/utilityApp.cpp
    src/lib_gui/utility/utilityPathDetection.cpp
    src/lib_gui/utility/path_detector/CombinedPathDetector.cpp
    src/lib_gui/utility/path_detector/PathDetector.cpp)

target_include_directories(
  Sourcetrail_lib_gui_utility
  PUBLIC
    src/lib_gui/utility/
    src/lib_gui/utility/path_detector/
    src/lib_gui/utility/path_detector/cxx_header/)

target_link_libraries(
  Sourcetrail_lib_gui_utility
  PRIVATE
    project_options
    project_warnings
    Boost::system
    Qt5::Core
    Sourcetrail::core
    Sourcetrail::lib
    Sourcetrail::lib_utility)
# Lib Gui ----------------------------------------------------------------------
# target for running versionnumber script
# workaround for running customcommand (ninja dependency cycle)
add_custom_target(versionnumber ALL)
# Lib Gui ----------------------------------------------------------------------
add_library(
  Sourcetrail_lib_gui
  ${LIB_GUI_FILES}
  bin/resources.qrc
  ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h)

add_library(Sourcetrail::lib_gui ALIAS Sourcetrail_lib_gui)

target_link_libraries(
  Sourcetrail_lib_gui
  PUBLIC
    Sourcetrail::lib_utility
    Sourcetrail::lib
    Qt5::Widgets
    Qt5::Network
    Qt5::Svg
    $<$<PLATFORM_ID:Windows>:Qt5::WinExtras>
  PRIVATE
    fmt::fmt
    Sourcetrail::core
    Sourcetrail::messaging
    $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
    Sourcetrail::lib_gui::utility)

# command for versioning script
add_custom_command(
  TARGET versionnumber
  PRE_BUILD
    COMMAND           ${CMAKE_COMMAND} -DBINARY_DIR=${CMAKE_BINARY_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/version.cmake
    BYPRODUCTS        ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h
    DEPENDS           Sourcetrail_lib_gui
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT           "check/update version number")

add_dependencies(Sourcetrail_lib_gui versionnumber)

target_include_directories(
  Sourcetrail_lib_gui
  PUBLIC
    ${LIB_GUI_INCLUDE_PATHS}
    ${LIB_INCLUDE_PATHS}
    ${CMAKE_BINARY_DIR}/src/lib_gui
    ${CMAKE_BINARY_DIR}/src/lib)

# configure platform specific include file
configure_file("${PROJECT_SOURCE_DIR}/src/lib_gui/platform_includes/includes.h.in"
               "${PROJECT_BINARY_DIR}/src/lib_gui/includes.h")

#configure the versioning file
configure_file(${CMAKE_SOURCE_DIR}/cmake/version.txt.in
               ${CMAKE_BINARY_DIR}/version.txt)

configure_file(${CMAKE_SOURCE_DIR}/cmake/productVersion.h.in
               ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h)

set_target_properties(
  Sourcetrail_lib_gui
  PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    INCLUDE_CURRENT_DIR ON)

set_property(SOURCE ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h
             PROPERTY SKIP_AUTOMOC ON)
# Indexer App ------------------------------------------------------------------
add_subdirectory(src/indexer)
# App --------------------------------------------------------------------------
add_subdirectory(src/app)
# Console ----------------------------------------------------------------------
add_subdirectory(src/console)
# Tests ------------------------------------------------------------------------
if(ENABLE_UNIT_TEST OR ENABLE_INTEGRATION_TEST OR ENABLE_E2E_TEST)
  if(WIN32)
    copy_qt_binaries("${CMAKE_BINARY_DIR}/test" FALSE)
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "make_directory" "${CMAKE_BINARY_DIR}/test")
  create_symlink("${CMAKE_SOURCE_DIR}/bin/test/data" "${CMAKE_BINARY_DIR}/test/data")

  find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS Test REQUIRED)

  if(ENABLE_UNIT_TEST)
    add_subdirectory(src/test)
    add_subdirectory(${CMAKE_SOURCE_DIR}/src/lib/tests/)
    add_subdirectory(${CMAKE_SOURCE_DIR}/src/lib_gui/tests/)
    if(BUILD_CXX_LANGUAGE_PACKAGE)
      add_subdirectory(${CMAKE_SOURCE_DIR}/src/lib_cxx/tests/)
    endif()
  endif()

  if(ENABLE_INTEGRATION_TEST OR ENABLE_E2E_TEST)
    add_subdirectory(tests)
  endif()
endif()
# Assets -----------------------------------------------------------------------
execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "make_directory" "${CMAKE_BINARY_DIR}/app")
create_symlink("${CMAKE_SOURCE_DIR}/bin/app/data" "${CMAKE_BINARY_DIR}/app/data")
create_symlink("${CMAKE_SOURCE_DIR}/bin/app/user" "${CMAKE_BINARY_DIR}/app/user")
# Pack -------------------------------------------------------------------------
if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

# Add other targets that you want installed here, be default we just package the one executable
# we know we want to ship
package_project(TARGETS Sourcetrail Sourcetrail_indexer
  # FIXME: this does not work! CK
  # PRIVATE_DEPENDENCIES_CONFIGURED project_options project_warnings
)

# Experience shows that explicit package naming can help make it easier to sort
# out potential ABI related issues before they start, while helping you
# track a build to a specific GIT SHA
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
