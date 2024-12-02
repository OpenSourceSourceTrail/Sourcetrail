# ${CMAKE_SOURCE_DIR}/src/core/utility/file/scopedTemporaryFile/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::ScopedTemporaryFile
  SOURCES
  ScopedTemporaryFile.cpp
  PUBLIC_HEADERS
  ScopedTemporaryFile.hpp
  PRIVATE_DEPS
  fmt::fmt
  Sourcetrail::core::utility::logging)
