# ${CMAKE_SOURCE_DIR}/src/core/utility/fileSystem/CMakeLists.txt
add_subdirectory(impls)

add_sourcetrail_library(
  NAME
  core::utility::IFileSystem
  SOURCES
  IFileSystem.cpp
  PUBLIC_HEADERS
  IFileSystem.hpp
  PUBLIC_DEPS
  nonstd::expected-lite)
