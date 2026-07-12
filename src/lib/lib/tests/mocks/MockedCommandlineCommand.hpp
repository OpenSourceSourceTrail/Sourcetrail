#pragma once
#include <gmock/gmock.h>

#include "CommandlineCommand.h"

struct MockedCommandlineCommand : public commandline::CommandlineCommand {
  using Args = std::vector<std::string>;
  using ReturnStatus = CommandlineCommand::ReturnStatus;

  MockedCommandlineCommand(std::string name, std::string description, commandline::CommandLineParser* parser)
      : commandline::CommandlineCommand(std::move(name), std::move(description), parser) {}
  MOCK_METHOD(void, setup, (), (override));
  MOCK_METHOD(ReturnStatus, parse, (Args&), (override));
  MOCK_METHOD(bool, hasHelp, (), (const, override));

  using commandline::CommandlineCommand::m_positional;
};