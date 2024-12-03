# ${CMAKE_SOURCE_DIR}/src/core/utility/commandline/commands/commandlineCommand/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::commandline::commands::CommandlineCommand
  core::utility::CommandlineCommand
  SOURCES
  CommandlineCommand.cpp
  PUBLIC_HEADERS
  CommandlineCommand.h
  PUBLIC_DEPS
  Boost::program_options)
