# ${CMAKE_SOURCE_DIR}/src/core/utility/commandline/commands/commandlineCommandConfig/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::commandline::CommandlineCommandConfig
  SOURCES
  CommandlineCommandConfig.cpp
  PUBLIC_HEADERS
  CommandlineCommandConfig.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::commandline::commands::CommandlineCommand
  PRIVATE_DEPS
  Sourcetrail::core::utility::commandline::CommandlineHelper
  Sourcetrail::core::utility::commandline::CommandLineParser
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::logging)
