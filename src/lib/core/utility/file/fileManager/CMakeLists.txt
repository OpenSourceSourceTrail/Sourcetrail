# ${CMAKE_SOURCE_DIR}/src/core/utility/file/fileManager/CMakeLists.txt
add_sourcetrail_library(
  NAME
  lib::core::utility::file::FileManager
  SOURCES
  FileManager.cpp
  PUBLIC_HEADERS
  FileManager.h
  PRIVATE_DEPS
  Sourcetrail::lib::core::utility::rangesTo
  Sourcetrail::lib::core::utility::file::FilePath
  Sourcetrail::lib::core::utility::file::FilePathFilter
  Sourcetrail::lib::core::utility::file::FileSystem)
