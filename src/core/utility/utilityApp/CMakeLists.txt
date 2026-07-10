# ${CMAKE_SOURCE_DIR}/src/core/utility/utilityApp/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::utilityApp
  SOURCES
  utilityApp.cpp
  PUBLIC_HEADERS
  utilityApp.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::ApplicationArchitectureType
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::OsType
  Boost::boost
  PRIVATE_DEPS
  Boost::filesystem
  Sourcetrail::core::utility::logging
  Sourcetrail::core::utility::ScopedFunctor
  Sourcetrail::core::utility::utilityString)
