# ${CMAKE_SOURCE_DIR}/src/core/utility/file/filePathFilter/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::FilePathFilter
  SOURCES
  FilePathFilter.cpp
  PUBLIC_HEADERS
  FilePathFilter.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::file::FilePath)
