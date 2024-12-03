# ${CMAKE_SOURCE_DIR}/src/core/utility/file/fileManager/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::FileManager
  SOURCES
  FileManager.cpp
  PUBLIC_HEADERS
  FileManager.h
  PRIVATE_DEPS
  range-v3::range-v3
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::file::FilePathFilter
  Sourcetrail::core::utility::file::FileSystem)
