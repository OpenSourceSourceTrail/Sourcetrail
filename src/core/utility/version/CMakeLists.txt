# ${CMAKE_SOURCE_DIR}/src/core/utility/version/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::Version
  SOURCES
  Version.cpp
  PUBLIC_HEADERS
  Version.h
  PRIVATE_DEPS
  fmt::fmt
  range-v3::range-v3
  Sourcetrail::core::utility::utilityString
  Sourcetrail::core::utility::logging)
