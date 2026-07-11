#include "CommandlineCommand.h"

#include <iostream>

namespace commandline {

CommandlineCommand::CommandlineCommand(std::string name, std::string description, CommandLineParser* parser)
    : m_name(std::move(name)), m_description(std::move(description)), m_parser(parser) {
  assert(parser != nullptr);
}

CommandlineCommand::~CommandlineCommand() = default;

void CommandlineCommand::printHelp() {
  std::cout << "Usage:\n\n  Sourcetrail " << m_name << " [option...]\n\n";
  std::cout << m_description << "\n\n";
  std::cout << m_options << std::endl;

  if(m_positional.max_total_count() > 0) {
    std::cout << "Positional Arguments: ";
    for(uint32_t i = 0; i < m_positional.max_total_count(); ++i) {
      std::cout << "\n  " << i + 1 << ": " << m_positional.name_for_position(i);
    }
    std::cout << std::endl;
  }
}

}    // namespace commandline