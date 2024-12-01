# ${CMAKE_SOURCE_DIR}/src/core/utility/commandline/commandlineHelper/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::commandline::CommandlineHelper
  SOURCES
  CommandlineHelper.cpp
  PUBLIC_HEADERS
  CommandlineHelper.h
  PRIVATE_DEPS
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::utilityString
  Boost::program_options
  fmt::fmt)
