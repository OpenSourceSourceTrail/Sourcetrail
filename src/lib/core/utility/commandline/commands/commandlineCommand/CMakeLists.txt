# ${PROJECT_SOURCE_DIR}/src/core/utility/commandline/commands/commandlineCommand/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::commandline::commands::CommandlineCommand lib::core::utility::CommandlineCommand
  SOURCES CommandlineCommand.cpp
  HEADERS CommandlineCommand.h
  PUBLIC_DEPS Boost::program_options)
