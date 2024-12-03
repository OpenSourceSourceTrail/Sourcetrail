#include "CommandlineHelper.h"

#include <fstream>
#include <string>

#include <boost/program_options.hpp>

#include <fmt/printf.h>

#include "FilePath.h"
#include "utilityString.h"

namespace commandline {

void parseConfigFile(po::variables_map& variablesMap, po::options_description& options) {
  if(variablesMap.count("config-file") != 0U) {
    const std::string configFile = variablesMap["config-file"].as<std::string>();
    std::ifstream ifs(configFile.c_str());

    if(!ifs) {
      fmt::print("Could not open config file({})\n", configFile);
    } else {
      po::store(po::parse_config_file(ifs, options), variablesMap);
    }
  }
}

std::vector<std::filesystem::path> extractPaths(const std::vector<std::string>& paths) {
  std::vector<std::filesystem::path> filePaths;
  for(const auto& pathString : paths) {
    const auto& temp = utility::splitToVector(pathString, ',');
    for(const auto& path : temp) {
      filePaths.emplace_back(path);
    }
  }
  return filePaths;
}

}    // namespace commandline
