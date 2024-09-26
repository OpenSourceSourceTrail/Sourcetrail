#include "CommandlineCommandConfig.h"

#include <functional>
#include <iostream>
#include <utility>

#include "CommandLineParser.h"
#include "CommandlineHelper.h"
#include "FilePath.h"
#include "IApplicationSettings.hpp"
#include "logging.h"

namespace commandline {
// helper functions
using intFunc = void (IApplicationSettings::*)(int);
void parseAndSetValue(intFunc func, const char* opt, IApplicationSettings* settings, po::variables_map& variablesMap) {
  if(variablesMap.count(opt) != 0U) {
    (settings->*func)(variablesMap[opt].as<int>());
  }
}

using boolFunc = void (IApplicationSettings::*)(bool);
void parseAndSetValue(boolFunc func, const char* opt, IApplicationSettings* settings, po::variables_map& variablesMap) {
  if(variablesMap.count(opt) != 0U) {
    (settings->*func)(variablesMap[opt].as<bool>());
  }
}

using filePathFunc = void (IApplicationSettings::*)(const FilePath&);
void parseAndSetValue(filePathFunc func, const char* opt, IApplicationSettings* settings, po::variables_map& variablesMap) {
  if(variablesMap.count(opt) != 0U) {
    FilePath path(variablesMap[opt].as<std::string>());
    if(!path.exists()) {
      std::cout << "\nWARNING: " << path.str() << " does not exist." << std::endl;
    }
    (settings->*func)(path);
  }
}

using vectorFuncStl = bool (IApplicationSettings::*)(const std::vector<std::filesystem::path>&);
void parseAndSetValue(vectorFuncStl func, const char* opt, IApplicationSettings* settings, po::variables_map& variablesMap) {
  if(variablesMap.count(opt) != 0U) {
    auto files = extractPaths(variablesMap[opt].as<std::vector<std::string>>());
    (settings->*func)(files);
  }
}

void printVector(const std::string& title, const std::vector<FilePath>& vec) {
  std::cout << "\n  " << title << ":";
  if(vec.empty()) {
    std::cout << "\n    -\n";
  }
  for(const FilePath& item : vec) {
    std::cout << "\n    " << item.str();
  }
}

CommandlineCommandConfig::CommandlineCommandConfig(CommandLineParser* parser)
    : CommandlineCommand("config", "Change preferences relevant to project indexing.", parser) {}

CommandlineCommandConfig::~CommandlineCommandConfig() = default;

void CommandlineCommandConfig::setup() {
  // clang-format off
  po::options_description options("Config Options");
  options.add_options()
    ("help,h", "Print this help message")
    ("indexer-threads,t",                 po::value<int>(),  "Set the number of threads used for indexing (0 uses ideal thread count)")
    ("use-processes,p",                   po::value<bool>(), "Enable C/C++ Indexer threads to run in different processes. <true/false>")
    ("logging-enabled,l",                 po::value<bool>(), "Enable file/console logging <true/false>")
    ("verbose-indexer-logging-enabled,L", po::value<bool>(), "Enable additional log of abstract syntax tree during the indexing. <true/false> WARNING Slows down indexing speed")
    ("global-header-search-paths,g",      po::value<std::vector<std::string>>(), "Global include paths (once per path or comma separated)")
    ("global-framework-search-paths,F",   po::value<std::vector<std::string>>(), "Global include paths (once per path or comma separated)")
    ("show,s", "displays all settings");
  // clang-format on
  m_options.add(options);
}

CommandlineCommand::ReturnStatus CommandlineCommandConfig::parse(std::vector<std::string>& args) {
  po::variables_map variablesMap;
  try {
    po::store(po::command_line_parser(args).options(m_options).run(), variablesMap);
    po::notify(variablesMap);

    parseConfigFile(variablesMap, m_options);
  } catch(po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
    std::cerr << m_options << std::endl;
    return ReturnStatus::CMD_FAILURE;
  }

  // when "sourcetrail config" without any options is started,
  // show help since configure nothing wont help
  if(variablesMap.count("help") != 0U || args.empty() || args[0] == "help") {
    printHelp();
    return ReturnStatus::CMD_QUIT;
  }

  auto* settings = IApplicationSettings::getInstanceRaw();
  if(settings == nullptr) {
    LOG_ERROR("No application settings loaded");
    return ReturnStatus::CMD_QUIT;
  }

  if(args[0] == "show" || variablesMap.count("show") != 0U) {
    std::cout << "Sourcetrail Settings:\n"
              << "\n  indexer-threads: " << settings->getIndexerThreadCount()
              << "\n  use-processes: " << settings->getMultiProcessIndexingEnabled()
              << "\n  logging-enabled: " << settings->getLoggingEnabled()
              << "\n  verbose-indexer-logging-enabled: " << settings->getVerboseIndexerLoggingEnabled();
    // printVector("global-header-search-paths", settings->getHeaderSearchPaths());
    // printVector("global-framework-search-paths", settings->getFrameworkSearchPaths());
    return ReturnStatus::CMD_QUIT;
  }

  // clang-format off
  parseAndSetValue(&IApplicationSettings::setMultiProcessIndexingEnabled,  "use-processes",                   settings, variablesMap);
  parseAndSetValue(&IApplicationSettings::setLoggingEnabled,               "logging-enabled",                 settings, variablesMap);
  parseAndSetValue(&IApplicationSettings::setVerboseIndexerLoggingEnabled, "verbose-indexer-logging-enabled", settings, variablesMap);
  parseAndSetValue(&IApplicationSettings::setIndexerThreadCount,           "indexer-threads",                 settings, variablesMap);
  parseAndSetValue(&IApplicationSettings::setHeaderSearchPaths,            "global-header-search-paths",      settings, variablesMap);
  parseAndSetValue(&IApplicationSettings::setFrameworkSearchPaths,         "global-framework-search-paths",   settings, variablesMap);
  // clang-format on

  settings->save();

  return ReturnStatus::CMD_QUIT;
}

}    // namespace commandline
