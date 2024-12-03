# ${CMAKE_SOURCE_DIR}/src/core/utility/commandline/commands/commandlineCommandIndex/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::commandline::CommandlineCommandIndex
  SOURCES
  CommandlineCommandIndex.cpp
  PUBLIC_HEADERS
  CommandlineCommandIndex.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::commandline::commands::CommandlineCommand
  PRIVATE_DEPS
  Sourcetrail::core::utility::commandline::CommandlineHelper
  Sourcetrail::core::utility::commandline::CommandLineParser)
