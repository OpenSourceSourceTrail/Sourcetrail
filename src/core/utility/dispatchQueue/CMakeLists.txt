# ${CMAKE_SOURCE_DIR}/src/core/utility/dispatchQueue/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::DispatchQueue
  SOURCES
  DispatchQueue.cpp
  DispatchRegistry.cpp
  PUBLIC_HEADERS
  DispatchQueue.h
  DispatchRegistry.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::GlobalId)
