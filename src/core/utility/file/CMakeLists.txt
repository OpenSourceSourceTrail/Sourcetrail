# ${CMAKE_SOURCE_DIR}/src/core/utility/file/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::FilePath
  SOURCES
  FilePath.cpp
  PUBLIC_HEADERS
  FilePath.h
  PRIVATE_DEPS
  Boost::filesystem
  Sourcetrail::core::utility::logging)
