# ${CMAKE_SOURCE_DIR}/src/core/utility/utilityApp/CMakeLists.txt
add_sourcetrail_library(
  NAME
  lib::core::utility::utilityApp
  SOURCES
  utilityApp.cpp
  PUBLIC_HEADERS
  utilityApp.h
  PUBLIC_DEPS
  Sourcetrail::lib::core::utility::ApplicationArchitectureType
  Sourcetrail::lib::core::utility::file::FilePath
  Sourcetrail::lib::core::utility::OsType
  Boost::boost
  PRIVATE_DEPS
  Boost::filesystem
  Sourcetrail::lib::core::utility::logging
  Sourcetrail::lib::core::utility::ScopedFunctor
  Sourcetrail::lib::core::utility::utilityString)
