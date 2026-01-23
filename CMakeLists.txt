#${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.23)
include(cmake/version.cmake)

project(
  Sourcetrail
  DESCRIPTION "interactive source explorer"
  VERSION ${VERSION_STRING}
  HOMEPAGE_URL "opensourcesourcetrail.github.io"
  LANGUAGES C CXX)
#---------------------------------------------------------------------------------------------------------------------------------
set(BUILD_CXX_LANGUAGE_PACKAGE
    OFF
    CACHE BOOL "Add C and C++ support to the Sourcetrail indexer.")
set(BUILD_DOC
    OFF
    CACHE BOOL "Build documentation.")
set(ENABLE_UNIT_TEST
    OFF
    CACHE BOOL "Build unit-tests.")
set(ENABLE_GUI_TEST
    OFF
    CACHE BOOL "Build gui-tests.")
set(ENABLE_INTEGRATION_TEST
    OFF
    CACHE BOOL "Build integration-tests.")
set(ENABLE_SANITIZER_ADDRESS
    OFF
    CACHE BOOL "Inject address sanitizer.")
set(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
    OFF
    CACHE BOOL "Inject undefined behavior sanitizer.")
set(ENABLE_SANITIZER_THREAD CACHE BOOL "Inject thread sanitizer. `not supported`")
set(ENABLE_SANITIZER_MEMORY
    OFF
    CACHE BOOL "Inject memory sanitizer. `not supported`")
set(ENABLE_COVERAGE
    OFF
    CACHE BOOL "Build coverage report run `ninja coverage`.")
set(ENABLE_BUILD_WITH_TIME_TRACE
    OFF
    CACHE BOOL "Trace building time.")
set(SOURCETRAIL_CMAKE_VERBOSE
    OFF
    CACHE BOOL "CMake verbose")
set(SOURCETRAIL_WARNING_AS_ERROR
    OFF
    CACHE BOOL "Treat warnings as errors")
set(SOURCETRAIL_USE_LIBCPP
    OFF
    CACHE BOOL "Use libc++ instead of libstdc++")

set(GCOV_VERSION
    "gcov-12"
    CACHE STRING "Version of gcov")
#---------------------------------------------------------------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
#---------------------------------------------------------------------------------------------------------------------------------
include(cmake/add_files.cmake)
include(cmake/version_setup.cmake)
include(cmake/licenses.cmake)
include(cmake/create_symlink.cmake)
include(cmake/add_sourcetrail_library.cmake)
include(cmake/add_sourcetrail_interface.cmake)
include(cmake/add_sourcetrail_test.cmake)
include(cmake/clang-tidy.cmake)
include(cmake/cppcheck.cmake)
include(cmake/coverage.cmake)
#---------------------------------------------------------------------------------------------------------------------------------
if(ENABLE_SANITIZER_ADDRESS
   OR ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
   OR ENABLE_SANITIZER_THREAD
   OR ENABLE_SANITIZER_MEMORY)
  include(cmake/sanitizers.cmake)
endif()
#---------------------------------------------------------------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  add_compile_options(-fcolor-diagnostics)
  if(SOURCETRAIL_USE_LIBCPP)
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
endif()
#---------------------------------------------------------------------------------------------------------------------------------
include(cmake/set_alternate_linker.cmake)
set(USE_ALTERNATE_LINKER
    ""
    CACHE STRING "Use alternate linker. Leave empty for system default; alternatives are 'gold', 'lld', 'bfd', 'mold'")
if(NOT
   "${USE_ALTERNATE_LINKER}"
   STREQUAL
   "")
  set_alternate_linker(${USE_ALTERNATE_LINKER})
endif()
#---------------------------------------------------------------------------------------------------------------------------------
include(cmake/compiler_warnings.cmake)
#---------------------------------------------------------------------------------------------------------------------------------
if(BUILD_DOC)
  include(cmake/doxygen.cmake)
endif()
# Variables ----------------------------------------------------------------------------------------------------------------------
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
  string(
    REGEX
    REPLACE "\n$"
            ""
            BASH
            "${BASH}")
  if(SOURCETRAIL_CMAKE_VERBOSE)
    message("bash: '${BASH}'")
  endif()
endif()
# Project ------------------------------------------------------------------------------------------------------------------------
set(PROJECT_NAME Sourcetrail)

#RPATH
if(UNIX AND NOT APPLE)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/lib/:$$ORIGIN/lib/")
endif()
# Settings -----------------------------------------------------------------------------------------------------------------------
find_package(expected-lite REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(SQLite3 CONFIG REQUIRED)
# Boost --------------------------------------------------------------------------------------------------------------------------
set(Boost_USE_MULTITHREAD ON)
set(Boost_USE_STATIC_LIBS
    ON
    CACHE BOOL "Set to ON to force the use of the static libraries.")
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(
  Boost 1.83
  COMPONENTS date_time
             filesystem
             program_options
             system
  REQUIRED)
# Qt -----------------------------------------------------------------------------------------------------------------------------
find_package(
  Qt6 6.8 REQUIRED
  COMPONENTS Widgets
             PrintSupport
             Network
             Svg
             Core5Compat)
# Setup --------------------------------------------------------------------------------------------------------------------------
if(WIN32)
  include(cmake/copy_qt_binaries.cmake)
endif()

if(ENABLE_UNIT_TEST OR ENABLE_INTEGRATION_TEST)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
  add_library(gtest_main ./tests/gtest_main.cpp)
  add_library(Sourcetrail::gtest_main ALIAS gtest_main)
  target_link_libraries(
    gtest_main
    PUBLIC GTest::gmock
           GTest::gtest
           spdlog::spdlog
           Sourcetrail::core::utility::utilityString
           Sourcetrail::core::utility::TextCodec)
endif()

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_PREFIX_LEN)

add_subdirectory(src/external)
add_subdirectory(src/lib)
add_subdirectory(src/lib_gui)
#---------------------------------------------------------------------------------------------------------------------------------
add_subdirectory(src/llm)
add_subdirectory(src/core)
add_subdirectory(src/scheduling)
add_subdirectory(src/messaging)
add_subdirectory(src/lib_gui/utility/utilityApp)
# Lib Cxx ------------------------------------------------------------------------------------------------------------------------
if(BUILD_CXX_LANGUAGE_PACKAGE)
  add_subdirectory(src/lib_cxx)
endif()
# Lib ----------------------------------------------------------------------------------------------------------------------------
add_library(Sourcetrail_lib ${LIB_FILES})
add_library(Sourcetrail::lib ALIAS Sourcetrail_lib)

target_include_directories(
  Sourcetrail_lib
  PUBLIC ${LIB_INCLUDE_PATHS}
         ${LIB_GUI_INCLUDE_PATHS}
         ${CMAKE_SOURCE_DIR}/src/lib_gui/utility/
         ${CMAKE_SOURCE_DIR}/src/lib_gui/utility/path_detector/
         ${CMAKE_BINARY_DIR}/src/lib)

target_link_libraries(
  Sourcetrail_lib
  PUBLIC Sourcetrail::messaging
         Sourcetrail::core::utility::ConfigManager
         Sourcetrail::core::utility::LowMemoryStringMap
         Sourcetrail::core::utility::Status
         Sourcetrail::core::utility::Tree
         Sourcetrail::core::utility::utility
         Sourcetrail::core::utility::Migration
         Sourcetrail::core::utility::Migrator
         Sourcetrail::core::utility::file::FilePath
         Sourcetrail::core::utility::file::FilePathFilter
         Sourcetrail::core::utility::toUnderlying
         Sourcetrail::scheduling
         CppSQLite::CppSQLite3
         nonstd::expected-lite
  PRIVATE Sourcetrail::core
          Sourcetrail::core::utility::ConfigManager
          Sourcetrail::core::utility::logging
          Sourcetrail::core::utility::ScopedFunctor
          Sourcetrail::core::utility::Status
          Sourcetrail::core::utility::TextAccess
          Sourcetrail::core::utility::TextCodec
          Sourcetrail::core::utility::utilityUuid
          Sourcetrail::lib_gui
          Sourcetrail::libGui::utility::utilityApp
          Sourcetrail::core::utility::file::FileSystem
          Sourcetrail::core::utility::file::utilityFile
          Sourcetrail::core::utility::file::FileManager
          ILLMService
          $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
          Boost::filesystem
          Boost::program_options
          Boost::system
          SQLite::SQLite3
          $<$<PLATFORM_ID:Windows>:bcrypt>)

#configure language package defines
configure_file("${CMAKE_SOURCE_DIR}/cmake/language_packages.h.in" "${CMAKE_BINARY_DIR}/src/lib/language_packages.h")

set_target_properties(
  Sourcetrail_lib
  PROPERTIES AUTOMOC ON)
# Lib gui_utility ----------------------------------------------------------------------------------------------------------------
add_library(Sourcetrail_lib_gui_utility)
add_library(Sourcetrail::lib_gui::utility ALIAS Sourcetrail_lib_gui_utility)

target_sources(
  Sourcetrail_lib_gui_utility
  PRIVATE src/lib_gui/utility/utilityPathDetection.cpp src/lib_gui/utility/path_detector/CombinedPathDetector.cpp
          src/lib_gui/utility/path_detector/PathDetector.cpp)

target_include_directories(Sourcetrail_lib_gui_utility PUBLIC src/lib_gui/utility/ src/lib_gui/utility/path_detector/
                                                              src/lib_gui/utility/path_detector/cxx_header/)

target_link_libraries(
  Sourcetrail_lib_gui_utility
  PUBLIC Sourcetrail::core::utility::ApplicationArchitectureType Sourcetrail::core::utility::OsType
  PRIVATE Boost::system
          Qt6::Core
          Sourcetrail::core
          Sourcetrail::lib
          Sourcetrail::core::utility::ScopedFunctor
          Sourcetrail::core::utility::logging
          Sourcetrail::libGui::utility::utilityApp)

# Lib Gui ------------------------------------------------------------------------------------------------------------------------
# target for running versionnumber script
# workaround for running customcommand (ninja dependency cycle)
add_custom_target(versionnumber ALL)
# Lib Gui ------------------------------------------------------------------------------------------------------------------------
add_library(Sourcetrail_lib_gui ${LIB_GUI_FILES} bin/resources.qrc ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h)

add_library(Sourcetrail::lib_gui ALIAS Sourcetrail_lib_gui)

target_link_libraries(
  Sourcetrail_lib_gui
  PUBLIC Sourcetrail::lib
         Qt6::Widgets
         Qt6::Network
         Qt6::Svg
         Sourcetrail::core::utility::SingleValueCache
         Sourcetrail::core::utility::LowMemoryStringMap
         Sourcetrail::core::utility::Migrator
         Sourcetrail::core::utility::file::FilePathFilter
         Sourcetrail::core::utility::file::FileSystem
         Sourcetrail::libGui::utility::utilityApp
  PRIVATE Sourcetrail::core
          Sourcetrail::core::utility::logging
          Sourcetrail::core::utility::OrderedCache
          Sourcetrail::core::utility::ScopedFunctor
          Sourcetrail::core::utility::Status
          Sourcetrail::core::utility::TextAccess
          Sourcetrail::core::utility::TextCodec
          Sourcetrail::core::utility::toUnderlying
          Sourcetrail::core::utility::utility
          Sourcetrail::core::utility::utilityUuid
          Sourcetrail::core::utility::file::utilityFile
          Sourcetrail::core::utility::file::FilePath
          Sourcetrail::core::utility::file::FileManager
          Sourcetrail::lib_gui::utility
          Sourcetrail::libGui::utility::utilityApp
          Sourcetrail::messaging
          Qt6::Core5Compat
          $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>)

# command for versioning script
add_custom_command(
  TARGET versionnumber
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${CMAKE_BINARY_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/version.cmake
  BYPRODUCTS ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h DEPENDS Sourcetrail_lib_gui
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "check/update version number")

add_dependencies(Sourcetrail_lib_gui versionnumber)

target_include_directories(
  Sourcetrail_lib_gui
  PUBLIC ${LIB_GUI_INCLUDE_PATHS}
         ${LIB_INCLUDE_PATHS}
         ${CMAKE_BINARY_DIR}/src/lib_gui
         ${CMAKE_BINARY_DIR}/src/lib)

# configure platform specific include file
configure_file("${PROJECT_SOURCE_DIR}/src/lib_gui/platform_includes/includes.h.in"
               "${PROJECT_BINARY_DIR}/src/lib_gui/includes.h")

#configure the versioning file
configure_file(${CMAKE_SOURCE_DIR}/cmake/productVersion.h.in ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h)

set_target_properties(
  Sourcetrail_lib_gui
  PROPERTIES AUTOMOC ON
             AUTORCC ON
             INCLUDE_CURRENT_DIR ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set_source_files_properties(${CMAKE_BINARY_DIR}/Sourcetrail_lib_gui_autogen/mocs_compilation.cpp
                              PROPERTIES COMPILE_FLAGS "-Wno-useless-cast")
endif()

set_property(SOURCE ${CMAKE_BINARY_DIR}/src/lib_gui/productVersion.h PROPERTY SKIP_AUTOMOC ON)
# Indexer App --------------------------------------------------------------------------------------------------------------------
add_subdirectory(src/indexer)
# App ----------------------------------------------------------------------------------------------------------------------------
add_subdirectory(src/app)
# Tests --------------------------------------------------------------------------------------------------------------------------
if(ENABLE_UNIT_TEST
   OR ENABLE_GUI_TEST
   OR ENABLE_INTEGRATION_TEST)
  if(WIN32 AND NOT ${SR_IS_CI})
    copy_qt_binaries("${CMAKE_BINARY_DIR}/test" FALSE)
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "make_directory" "${CMAKE_BINARY_DIR}/test")
  create_symlink("${CMAKE_SOURCE_DIR}/bin/test/data" "${CMAKE_BINARY_DIR}/test/data")

  find_package(
    Qt6 6.8
    COMPONENTS Test Sql
    REQUIRED)

  if(ENABLE_UNIT_TEST)
    add_subdirectory(${CMAKE_SOURCE_DIR}/src/lib/tests/)
    add_subdirectory(${CMAKE_SOURCE_DIR}/src/lib_gui/tests/)
    if(BUILD_CXX_LANGUAGE_PACKAGE)
      add_subdirectory(${CMAKE_SOURCE_DIR}/src/lib_cxx/tests/)
    endif()
  endif()

  if(ENABLE_INTEGRATION_TEST)
    add_subdirectory(tests)
  endif()
endif()
# Assets -------------------------------------------------------------------------------------------------------------------------
execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "make_directory" "${CMAKE_BINARY_DIR}/app")
create_symlink("${CMAKE_SOURCE_DIR}/bin/app/data" "${CMAKE_BINARY_DIR}/app/data")
create_symlink("${CMAKE_SOURCE_DIR}/bin/app/user" "${CMAKE_BINARY_DIR}/app/user")
# Install Executables ------------------------------------------------------------------------------------------------------------
set(APPIMAGE_ROOT "usr")
set(INSTALL_RUNTIME_DIR "${APPIMAGE_ROOT}/bin")

install(TARGETS Sourcetrail Sourcetrail_indexer RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR})

install(DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app/data/ DESTINATION ${INSTALL_RUNTIME_DIR}/data/)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app/user/ DESTINATION ${INSTALL_RUNTIME_DIR}/user/)

# Install desktop file for application
install(FILES "${CMAKE_SOURCE_DIR}/cmake/Sourcetrail.desktop" DESTINATION "${APPIMAGE_ROOT}/share/applications")

# Install application icon for desktop
install(FILES "${CMAKE_SOURCE_DIR}/bin/app/data/gui/installer/Sourcetrail.png"
        DESTINATION "${APPIMAGE_ROOT}/share/icons/hicolor/256x256/apps")
# Packaging ----------------------------------------------------------------------------------------------------------------------
# TODO(SOUR-104)
