# ${CMAKE_SOURCE_DIR}/src/lib_utility/utility/CMakeLists.txt
add_subdirectory(file)
add_subdirectory(logging)

add_sourcetrail_library(
  NAME
  core::utility::utilityString
  SOURCES
  utilityString.cpp
  PUBLIC_HEADERS
  utilityString.h
  PRIVATE_DEPS
  Boost::system)

add_sourcetrail_library(
  NAME
  core::utility::TextCodec
  SOURCES
  TextCodec.cpp
  PUBLIC_HEADERS
  TextCodec.h
  PRIVATE_DEPS
  Qt5::Core)
