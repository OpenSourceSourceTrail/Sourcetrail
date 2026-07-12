# ${CMAKE_SOURCE_DIR}/src/lib_utility/ConfigManager/CMakeLists.txt
add_sourcetrail_library(
  NAME
  lib::core::utility::ConfigManager
  SOURCES
  ConfigManager.cpp
  PUBLIC_HEADERS
  ConfigManager.hpp
  PUBLIC_DEPS
  Sourcetrail::lib::core::utility::file::FilePath
  Sourcetrail::lib::core::utility::utilityString
  PRIVATE_DEPS
  Sourcetrail::lib::core::utility::logging
  Sourcetrail::lib::core::utility::utility
  Sourcetrail::lib::core::utility::TextAccess
  Boost::system
  range-v3::range-v3
  Sourcetrail::lib::core)
