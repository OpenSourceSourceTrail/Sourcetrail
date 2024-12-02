# ${CMAKE_SOURCE_DIR}/src/core/utility/file/fileTree/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::FileTree
  SOURCES
  FileTree.cpp
  PUBLIC_HEADERS
  FileTree.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::file::FilePath
  PRIVATE_DEPS
  Sourcetrail::core::utility::file::FileSystem)
