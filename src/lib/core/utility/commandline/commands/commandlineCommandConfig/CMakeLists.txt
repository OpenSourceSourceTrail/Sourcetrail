# ${CMAKE_SOURCE_DIR}/src/core/utility/commandline/commands/commandlineCommandConfig/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::commandline::CommandlineCommandConfig
  SOURCES CommandlineCommandConfig.cpp
  PUBLIC_HEADERS CommandlineCommandConfig.h
  PUBLIC_DEPS Sourcetrail::lib::core::utility::commandline::commands::CommandlineCommand
  PRIVATE_DEPS
    Sourcetrail::lib::core::utility::commandline::CommandlineHelper
    Sourcetrail::lib::core::utility::commandline::CommandLineParser Sourcetrail::lib::core::utility::file::FilePath
    Sourcetrail::lib::core::utility::logging)
