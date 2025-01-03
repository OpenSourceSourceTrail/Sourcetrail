#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "CommandlineCommandIndex.h"
#include "CommandLineParser.h"
#include "IApplicationSettings.hpp"
#include "utilities/CollectOutStream.hpp"
#include "utilities/FileHandler.hpp"

using namespace testing;
using namespace commandline;

struct CommandlineCommandIndexFix : public Test {
  void SetUp() override {
    mParser = std::make_unique<CommandLineParser>("2023.6.8");

    mConfig = std::make_unique<CommandlineCommandIndex>(mParser.get());
    mConfig->setup();
  }

  std::unique_ptr<CommandLineParser> mParser;    // TODO: Mock that
  std::unique_ptr<CommandlineCommandIndex> mConfig;
};

TEST_F(CommandlineCommandIndexFix, emptyArgs) {
  std::vector<std::string> args;

  constexpr std::string_view Result =
      "Usage:\n\n  Sourcetrail index [option...]\n\nIndex a certain project.\n\n\nConfig Options:\n  -h [ --help ]          "
      "Print this help message\n  -i [ --incomplete ]    Also reindex incomplete files (files with errors)\n  -f [ --full ]      "
      "    Index full project (omit to only index new/changed \n                         files)\n  -s [ --shallow ]       Build "
      "a shallow index is supported by the project\n  --project-file arg     Project file to index (.srctrlprj)\n\nPositional "
      "Arguments: \n  1: project-file\n";

  CollectOutStream oStream(std::cout);
  auto ret = mConfig->parse(args);
  oStream.close();
  ASSERT_EQ(CommandlineCommand::ReturnStatus::CMD_QUIT, ret);
  ASSERT_THAT(oStream.str(), StrEq(Result));
}

TEST_F(CommandlineCommandIndexFix, helpArgs) {
  std::vector<std::string> args = {"--help"};

  constexpr std::string_view Result =
      "Usage:\n\n  Sourcetrail index [option...]\n\nIndex a certain project.\n\n\nConfig Options:\n  -h [ --help ]          "
      "Print this help message\n  -i [ --incomplete ]    Also reindex incomplete files (files with errors)\n  -f [ --full ]      "
      "    Index full project (omit to only index new/changed \n                         files)\n  -s [ --shallow ]       Build "
      "a shallow index is supported by the project\n  --project-file arg     Project file to index (.srctrlprj)\n\nPositional "
      "Arguments: \n  1: project-file\n";

  CollectOutStream oStream(std::cout);
  auto ret = mConfig->parse(args);
  oStream.close();
  ASSERT_EQ(CommandlineCommand::ReturnStatus::CMD_QUIT, ret);
  ASSERT_THAT(oStream.str(), StrEq(Result));
}

TEST_F(CommandlineCommandIndexFix, goodCaseArgs) {
  std::vector<std::string> args = {"-f", "-s"};

  auto ret = mConfig->parse(args);

  ASSERT_EQ(CommandlineCommand::ReturnStatus::CMD_OK, ret);
  EXPECT_EQ(RefreshMode::AllFiles, mParser->getRefreshMode());
  EXPECT_TRUE(mParser->getShallowIndexingRequested());
}

TEST_F(CommandlineCommandIndexFix, projectIsAbsent) {
  auto handler = FileHandler::createEmptyFile("/tmp/invalid");
  std::vector<std::string> args = {"/tmp/invalid"};

  auto ret = mConfig->parse(args);

  constexpr auto Result = L"Provided Projectfile is not valid:\n* Provided Projectfile('invalid')  has a wrong file ending";

  ASSERT_EQ(CommandlineCommand::ReturnStatus::CMD_OK, ret);
  ASSERT_EQ(mParser->getError(), Result);
}

TEST_F(CommandlineCommandIndexFix, projectIsInvalidProj) {
  auto handler = FileHandler::createEmptyFile("/tmp/invalid.srctrlprj");
  std::vector<std::string> args = {"/tmp/invalid.srctrlprj"};

  auto ret = mConfig->parse(args);

  constexpr auto Result =
      L"Provided Projectfile is not valid:\n* Provided Projectfile('invalid.srctrlprj')  could not be loaded (invalid)";

  ASSERT_EQ(CommandlineCommand::ReturnStatus::CMD_OK, ret);
  ASSERT_EQ(mParser->getError(), Result);
}

TEST_F(CommandlineCommandIndexFix, projectArgs) {
  std::vector<std::string> args = {"--project-file", "app/user/projects/tutorial/tutorial.srctrlprj"};

  auto ret = mConfig->parse(args);

  ASSERT_EQ(CommandlineCommand::ReturnStatus::CMD_OK, ret);
}