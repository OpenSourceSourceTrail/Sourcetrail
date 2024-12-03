# ${CMAKE_SOURCE_DIR}/src/core/utility/TextAccess/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::TextAccess
  SOURCES
  TextAccess.cpp
  PUBLIC_HEADERS
  TextAccess.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::file::FilePath
  PRIVATE_DEPS
  Sourcetrail::core::utility::logging)
