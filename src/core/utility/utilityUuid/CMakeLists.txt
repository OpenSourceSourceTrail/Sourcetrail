# ${CMAKE_SOURCE_DIR}/src/core/utility/utilityUuid/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::utilityUuid
  SOURCES
  utilityUuid.cpp
  PUBLIC_HEADERS
  utilityUuid.h
  PRIVATE_DEPS
  Boost::system)
