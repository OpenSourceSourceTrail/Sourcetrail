#pragma once

#include "CommandlineCommand.h"

namespace commandline {

class CommandlineCommandIndex : public CommandlineCommand {
public:
  explicit CommandlineCommandIndex(CommandLineParser* parser);

  CommandlineCommandIndex(const CommandlineCommandIndex&) = delete;
  CommandlineCommandIndex& operator=(const CommandlineCommandIndex&) = delete;
  CommandlineCommandIndex(CommandlineCommandIndex&&) = delete;
  CommandlineCommandIndex& operator=(CommandlineCommandIndex&&) = delete;

  ~CommandlineCommandIndex() override;

  void setup() override;

  ReturnStatus parse(std::vector<std::string>& args) override;

  [[nodiscard]] bool hasHelp() const override {
    return true;
  }
};

}    // namespace commandline