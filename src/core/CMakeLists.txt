#${CMAKE_SOURCE_DIR}/src/core/CMakeLists.txt
add_library(Sourcetrail_core)
add_library(Sourcetrail::core ALIAS Sourcetrail_core)

target_sources(
  Sourcetrail_core
  PRIVATE ${CMAKE_SOURCE_DIR}/src/lib/utility/file/FileInfo.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/file/FileManager.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/file/FilePath.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/file/FilePathFilter.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/file/FileRegister.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/file/FileSystem.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/file/FileTree.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/file/ScopedTemporaryFile.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/file/utilityFile.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/ConfigManager.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/ScopedFunctor.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/TimeStamp.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/tracing.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/utility.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/utilityUuid.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/Version.cpp
          ${CMAKE_SOURCE_DIR}/src/lib/utility/text/TextAccess.cpp)

target_include_directories(
  Sourcetrail_core
  PUBLIC ${CMAKE_SOURCE_DIR}/src/lib/utility/
         ${CMAKE_SOURCE_DIR}/src/lib/app/paths/
         ${CMAKE_SOURCE_DIR}/src/lib/utility/file/
         ${CMAKE_SOURCE_DIR}/src/lib/utility/logging/
         ${CMAKE_SOURCE_DIR}/src/lib/utility/text/)

target_link_libraries(Sourcetrail_core PUBLIC Boost::date_time Boost::system Boost::filesystem)

target_link_libraries(
  Sourcetrail_core
  PUBLIC fmt::fmt
         range-v3::range-v3
         Sourcetrail::lib_utility
         spdlog::spdlog
  PRIVATE Boost::filesystem
          Sourcetrail::lib_utility
          tinyxml::tinyxml
          fmt::fmt)

# $<$<PLATFORM_ID:Windows>:bcrypt>
target_compile_definitions(
  Sourcetrail_core
  PUBLIC $<$<PLATFORM_ID:Windows>:D_WINDOWS>
         $<$<PLATFORM_ID:Linux>:D_LINUX>
         $<$<PLATFORM_ID:Darwin>:D_Darwin>
         $<$<PLATFORM_ID:Windows>:SPDLOG_WCHAR_TO_UTF8_SUPPORT>)

myproject_set_project_warnings(
  Sourcetrail_core
  ${SOURCETRAIL_WARNING_AS_ERROR}
  ""
  ""
  ""
  "")

if(ENABLE_UNIT_TEST)
  add_subdirectory(tests)
endif()
