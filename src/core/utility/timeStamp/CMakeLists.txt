# ${CMAKE_SOURCE_DIR}/src/core/utility/timeStamp/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::TimeStamp
  SOURCES
  TimeStamp.cpp
  PUBLIC_HEADERS
  TimeStamp.h
  PUBLIC_DEPS
  Boost::date_time)
