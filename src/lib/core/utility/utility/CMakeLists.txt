# ${CMAKE_SOURCE_DIR}/src/core/utility/utility/CMakeLists.txt
add_sourcetrail_library(
  NAME
  lib::core::utility::utility
  SOURCES
  utility.cpp
  PUBLIC_HEADERS
  utility.h
  PUBLIC_DEPS
  Sourcetrail::lib::core::utility::file::FilePath
  Sourcetrail::lib::core::utility::utilityString
  PRIVATE_DEPS
  range-v3::range-v3)
