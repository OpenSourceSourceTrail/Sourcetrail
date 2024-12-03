# ${CMAKE_SOURCE_DIR}/src/core/utility/utility/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::utility
  SOURCES
  utility.cpp
  PUBLIC_HEADERS
  utility.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::utilityString
  PRIVATE_DEPS
  range-v3::range-v3)
