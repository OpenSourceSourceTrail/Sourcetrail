#pragma once

#include "CommandlineCommand.h"

namespace commandline {

class CommandlineCommandConfig final : public CommandlineCommand {
public:
  explicit CommandlineCommandConfig(CommandLineParser* parser);

  CommandlineCommandConfig(const CommandlineCommandConfig&) = delete;
  CommandlineCommandConfig& operator=(const CommandlineCommandConfig&) = delete;
  CommandlineCommandConfig(CommandlineCommandConfig&&) = delete;
  CommandlineCommandConfig& operator=(CommandlineCommandConfig&&) = delete;

  ~CommandlineCommandConfig() override;

  void setup() override;

  ReturnStatus parse(std::vector<std::string>& args) override;

  [[nodiscard]] bool hasHelp() const override {
    return true;
  }
};

}    // namespace commandline
