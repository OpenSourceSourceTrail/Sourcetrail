# ${CMAKE_SOURCE_DIR}/src/core/utility/commandline/commands/commandlineCommandIndex/CMakeLists.txt
add_sourcetrail_library(
  NAME
  lib::core::utility::commandline::CommandlineCommandIndex
  SOURCES
  CommandlineCommandIndex.cpp
  PUBLIC_HEADERS
  CommandlineCommandIndex.h
  PUBLIC_DEPS
  Sourcetrail::lib::core::utility::commandline::commands::CommandlineCommand
  PRIVATE_DEPS
  Sourcetrail::lib::core::utility::commandline::CommandlineHelper
  Sourcetrail::lib::core::utility::commandline::CommandLineParser)
