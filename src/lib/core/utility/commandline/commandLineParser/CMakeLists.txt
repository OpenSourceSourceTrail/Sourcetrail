# ${PROJECT_SOURCE_DIR}/src/core/utility/commandline/commandLineParser/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::commandline::CommandLineParser
  SOURCES CommandLineParser.cpp
  HEADERS CommandLineParser.h
  PUBLIC_DEPS Boost::program_options Sourcetrail::lib::core::utility::file::FilePath Sourcetrail::lib::lib
  PRIVATE_DEPS
    Sourcetrail::lib::core::utility::ConfigManager
    Sourcetrail::lib::core::utility::commandline::CommandlineCommandConfig
    Sourcetrail::lib::core::utility::commandline::CommandlineCommandIndex Sourcetrail::lib::core::utility::TextAccess)
