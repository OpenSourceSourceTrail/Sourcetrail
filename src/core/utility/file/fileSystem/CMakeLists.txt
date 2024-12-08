# ${CMAKE_SOURCE_DIR}/src/core/utility/file/fileSystem/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::FileSystem
  SOURCES
  FileSystem.cpp
  PUBLIC_HEADERS
  FileSystem.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::file::FileInfo
  Sourcetrail::core::utility::TimeStamp
  PRIVATE_DEPS
  Boost::system
  Sourcetrail::core::utility::utilityString
  Sourcetrail_core_utility_logging)
