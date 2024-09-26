#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace boost::program_options {
class variables_map;
class options_description;
}    // namespace boost::program_options

class FilePath;

namespace po = boost::program_options;

namespace commandline {

void parseConfigFile(po::variables_map& variablesMap, po::options_description& options);

std::vector<std::filesystem::path> extractPaths(const std::vector<std::string>& paths);

}    // namespace commandline
