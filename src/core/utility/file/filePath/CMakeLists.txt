# ${CMAKE_SOURCE_DIR}/src/core/utility/file/filePath/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::FilePath
  SOURCES
  FilePath.cpp
  PUBLIC_HEADERS
  FilePath.h
  PRIVATE_DEPS
  Sourcetrail::core::utility::logging
  Sourcetrail_core_utility_file_FileSystem)
