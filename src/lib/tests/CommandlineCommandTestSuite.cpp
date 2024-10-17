#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "CommandlineCommand.h"
#include "CommandLineParser.h"
#include "mocks/MockedCommandlineCommand.hpp"
#include "utilities/CollectOutStream.hpp"

using namespace testing;

struct CommandlineCommand : Test {
  void SetUp() override {
    parser = std::make_unique<commandline::CommandLineParser>("");
    const std::string name = "name";
    const std::string desc = "desc";
    mockedCommandLine = std::make_unique<StrictMock<MockedCommandlineCommand>>(name, desc, parser.get());

    EXPECT_THAT(mockedCommandLine->name(), StrEq(name));
    EXPECT_THAT(mockedCommandLine->description(), StrEq(desc));
    EXPECT_THAT(mockedCommandLine->description(), StrEq(desc));
  }

  std::unique_ptr<commandline::CommandLineParser> parser;
  std::unique_ptr<StrictMock<MockedCommandlineCommand>> mockedCommandLine;
};

TEST_F(CommandlineCommand, emptyHelp) {
  CollectOutStream collectStreamBefore(std::cout);
  mockedCommandLine->printHelp();
  collectStreamBefore.close();
  constexpr std::string_view HelpMessageBefore = "Usage:\n\n  Sourcetrail name [option...]\n\ndesc\n\n\n";
  EXPECT_THAT(collectStreamBefore.str(), StrEq(HelpMessageBefore));
}

TEST_F(CommandlineCommand, AddPosToHelp) {
  mockedCommandLine->m_positional.add("pos", 1);

  CollectOutStream collectStream(std::cout);
  mockedCommandLine->printHelp();
  collectStream.close();
  constexpr std::string_view HelpMessage =
      "Usage:\n\n  Sourcetrail name [option...]\n\ndesc\n\n\nPositional Arguments: \n  1: pos\n";
  EXPECT_THAT(collectStream.str(), StrEq(HelpMessage));
}