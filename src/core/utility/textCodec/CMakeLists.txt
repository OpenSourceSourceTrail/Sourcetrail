# ${CMAKE_SOURCE_DIR}/src/core/utility/TextCodec/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::TextCodec
  SOURCES
  TextCodec.cpp
  PUBLIC_HEADERS
  TextCodec.h
  PRIVATE_DEPS
  Qt6::Core
  Qt6::Core5Compat)
