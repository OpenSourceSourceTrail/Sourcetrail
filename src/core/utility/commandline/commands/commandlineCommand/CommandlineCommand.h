#pragma once

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace commandline {

class CommandLineParser;

class CommandlineCommand {
public:
  enum class ReturnStatus { CMD_OK, CMD_QUIT, CMD_FAILURE };

  CommandlineCommand(std::string name, std::string description, CommandLineParser* parser);

  CommandlineCommand(const CommandlineCommand&) = delete;
  CommandlineCommand& operator=(const CommandlineCommand&) = delete;
  CommandlineCommand(CommandlineCommand&&) = delete;
  CommandlineCommand& operator=(CommandlineCommand&&) = delete;

  virtual ~CommandlineCommand();

  const std::string& name() {
    return m_name;
  }

  const std::string& description() {
    return m_description;
  }

  virtual void setup() = 0;

  virtual ReturnStatus parse(std::vector<std::string>& args) = 0;

  [[nodiscard]] virtual bool hasHelp() const = 0;

  virtual void printHelp();

protected:
  const std::string m_name;
  const std::string m_description;
  CommandLineParser* m_parser;

  po::options_description m_options;
  po::positional_options_description m_positional;
};

}    // namespace commandline