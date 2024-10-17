#include "CommandlineCommandIndex.h"

#include <iostream>

#include "CommandlineHelper.h"
#include "CommandLineParser.h"

namespace po = boost::program_options;

namespace commandline {

CommandlineCommandIndex::CommandlineCommandIndex(CommandLineParser* parser)
    : CommandlineCommand("index", "Index a certain project.", parser) {}

CommandlineCommandIndex::~CommandlineCommandIndex() = default;

void CommandlineCommandIndex::setup() {
  po::options_description options("Config Options");
  // clang-format off
  options.add_options()
    ("help,h", "Print this help message")
    ("incomplete,i", "Also reindex incomplete files (files with errors)")
    ("full,f", "Index full project (omit to only index new/changed files)")
    ("shallow,s", "Build a shallow index is supported by the project")
    ("project-file", po::value<std::string>(), "Project file to index (.srctrlprj)");
  // clang-format on
  m_options.add(options);
  m_positional.add("project-file", 1);
}

CommandlineCommand::ReturnStatus CommandlineCommandIndex::parse(std::vector<std::string>& args) {
  po::variables_map variablesMap;
  try {
    po::store(po::command_line_parser(args).options(m_options).positional(m_positional).run(), variablesMap);
    po::notify(variablesMap);

    parseConfigFile(variablesMap, m_options);
  } catch(po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
    std::cerr << m_options << std::endl;
    return ReturnStatus::CMD_FAILURE;
  }

  if((variablesMap.count("help") != 0U) || args.empty() || args[0] == "help") {
    printHelp();
    return ReturnStatus::CMD_QUIT;
  }

  if(variablesMap.count("full") != 0U) {
    m_parser->fullRefresh();
  } else if(variablesMap.count("incomplete") != 0U) {
    m_parser->incompleteRefresh();
  }

  if(variablesMap.count("shallow") != 0U) {
    m_parser->setShallowIndexingRequested();
  }

  if(variablesMap.count("project-file") != 0U) {
    m_parser->setProjectFile(FilePath(variablesMap["project-file"].as<std::string>()));
  }

  return ReturnStatus::CMD_OK;
}

}    // namespace commandline