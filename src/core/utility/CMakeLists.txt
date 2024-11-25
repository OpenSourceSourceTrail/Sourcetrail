# ${CMAKE_SOURCE_DIR}/src/lib_utility/utility/CMakeLists.txt
add_subdirectory(file)
add_subdirectory(logging)

add_sourcetrail_library(
  NAME
  core::utility::ConfigManager
  SOURCES
  ConfigManager.cpp
  PUBLIC_HEADERS
  ConfigManager.hpp
  PUBLIC_DEPS
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::utilityString
  Sourcetrail::core::utility::logging
  PRIVATE_DEPS
  Boost::system
  range-v3::range-v3
  Sourcetrail::core)

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

add_sourcetrail_library(
  NAME
  core::utility::TimeStamp
  SOURCES
  TimeStamp.cpp
  PUBLIC_HEADERS
  TimeStamp.h
  PUBLIC_DEPS
  Boost::date_time)
