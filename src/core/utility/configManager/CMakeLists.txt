# ${CMAKE_SOURCE_DIR}/src/lib_utility/ConfigManager/CMakeLists.txt
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
  PRIVATE_DEPS
  Sourcetrail::core::utility::logging
  Sourcetrail::core::utility::utility
  Sourcetrail::core::utility::TextAccess
  Boost::system
  range-v3::range-v3
  Sourcetrail::core)
