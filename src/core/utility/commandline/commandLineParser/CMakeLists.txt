# ${CMAKE_SOURCE_DIR}/src/core/utility/commandline/commandLineParser/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::commandline::CommandLineParser
  SOURCES
  CommandLineParser.cpp
  PUBLIC_HEADERS
  CommandLineParser.h
  PUBLIC_DEPS
  Boost::program_options
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::lib
  PRIVATE_DEPS
  Sourcetrail::core::utility::ConfigManager
  Sourcetrail::core::utility::commandline::CommandlineCommandConfig
  Sourcetrail::core::utility::commandline::CommandlineCommandIndex
  Sourcetrail::core::utility::TextAccess)
