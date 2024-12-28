# ${CMAKE_SOURCE_DIR}/src/core/utility/fileSystem/impls/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::FileSystem
  SOURCES
  FileSystem.cpp
  PUBLIC_HEADERS
  FileSystem.hpp
  PUBLIC_DEPS
  Sourcetrail::core::utility::IFileSystem
  PRIVATE_DEPS
  fmt::fmt)
