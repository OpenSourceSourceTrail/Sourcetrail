# ${CMAKE_SOURCE_DIR}/src/core/utility/utilityString/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::utilityString
  SOURCES
  utilityString.cpp
  PUBLIC_HEADERS
  utilityString.h
  PRIVATE_DEPS
  Boost::system)
