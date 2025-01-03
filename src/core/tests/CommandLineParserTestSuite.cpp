#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "CommandLineParser.h"
#include "utilities/CollectOutStream.hpp"
#include "utilities/FileHandler.hpp"
#include "utilityString.h"

using namespace ::testing;
using namespace std::string_literals;

struct CommandLineParserFix : Test {
  void SetUp() override {
    parser = std::make_unique<commandline::CommandLineParser>("0.1.0");
  }
  std::unique_ptr<commandline::CommandLineParser> parser;
};

TEST_F(CommandLineParserFix, goodCase) {}

// NOLINTNEXTLINE
TEST(CommandLineParser, refreshModes) {
  commandline::CommandLineParser parser({});
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());

  parser.fullRefresh();
  EXPECT_EQ(RefreshMode::AllFiles, parser.getRefreshMode());

  parser.incompleteRefresh();
  EXPECT_EQ(RefreshMode::UpdatedAndIncompleteFiles, parser.getRefreshMode());
}

// NOLINTNEXTLINE
TEST(CommandLineParser, ShallowIndexing) {
  commandline::CommandLineParser parser({});
  EXPECT_FALSE(parser.getShallowIndexingRequested());

  parser.setShallowIndexingRequested(true);
  EXPECT_TRUE(parser.getShallowIndexingRequested());

  parser.setShallowIndexingRequested(false);
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}

// NOLINTNEXTLINE
TEST(CommandLineParser, initStateEmptyArgs) {
  std::vector<std::string> args;
  commandline::CommandLineParser parser({});
  parser.preparse(args);
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_FALSE(parser.exitApplication());
  EXPECT_FALSE(parser.hasError());
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, helpOption) {
  std::vector<std::string> args{"--help"};
  commandline::CommandLineParser parser({});
  {
    constexpr std::string_view HelpString =
        "Usage:\n  Sourcetrail [command] [option...] [positional arguments]\n\nCommands:\n  config                 Change "
        "preferences relevant to project indexing.*\n  index                  Index a certain project.*\n\n  * has its own "
        "--help\n\nOptions:\n  -h [ --help ]          Print this help message\n  -v [ --version ]       Version of Sourcetrail\n "
        " --project-file arg     Open Sourcetrail with this project (.srctrlprj)\n\nPositional Arguments: \n  1: project-file\n";
    CollectOutStream collectCout(std::cout);
    parser.preparse(args);
    collectCout.close();
    EXPECT_THAT(collectCout.str(), StrEq(HelpString));
  }
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_TRUE(parser.exitApplication());
  EXPECT_FALSE(parser.hasError());
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, versionOption) {
  constexpr std::string_view VersionString = "Sourcetrail Version 2023.6.8\n";
  std::vector<std::string> args{"--version"};
  commandline::CommandLineParser parser("2023.6.8");
  {
    CollectOutStream collectCout(std::cout);
    parser.preparse(args);
    collectCout.close();
    EXPECT_THAT(collectCout.str(), VersionString);
  }
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_TRUE(parser.exitApplication());
  EXPECT_FALSE(parser.hasError());
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, projectFileOptionIsEmpty) {
  constexpr std::string_view MissingProjectString =
      "ERROR: the required argument for option '--project-file' is missing\n\n\nOptions:\n  -h [ --help ]          Print this "
      "help message\n  -v [ --version ]       Version of Sourcetrail\n  --project-file arg     Open Sourcetrail with this "
      "project (.srctrlprj)\n\n";
  std::vector<std::string> args{"--project-file"};
  commandline::CommandLineParser parser({});
  {
    CollectOutStream collectCout(std::cout);
    CollectOutStream collectCerr(std::cerr);
    parser.preparse(args);
    collectCout.close();
    collectCerr.close();
    EXPECT_THAT(collectCout.str(), IsEmpty());
    EXPECT_THAT(collectCerr.str(), StrEq(MissingProjectString));
  }
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_FALSE(parser.exitApplication());
  EXPECT_FALSE(parser.hasError());
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, projectFileOptionNotExists) {
  std::vector<std::string> args{"--project-file", "not-exists"};
  commandline::CommandLineParser parser({});
  parser.preparse(args);
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_FALSE(parser.exitApplication());
  EXPECT_TRUE(parser.hasError());
  const std::wstring GeneratedErrorMessage =
      L"Provided Projectfile is not valid:\n* Provided Projectfile('not-exists')  does not exist";
  EXPECT_THAT(parser.getError(), StrEq(GeneratedErrorMessage));
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, projectFileMissingExtention) {
  constexpr auto EmptyProjectPath = "/tmp/empty.proj";
  std::vector<std::string> args{"--project-file", EmptyProjectPath};
  auto fileHandler = FileHandler::createEmptyFile(args.back());

  commandline::CommandLineParser parser({});
  parser.preparse(args);
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_FALSE(parser.exitApplication());
  EXPECT_TRUE(parser.hasError());
  const auto ErrorMessage = utility::decodeFromUtf8("Provided Projectfile is not valid:\n* Provided Projectfile('"s +
                                                    fs::path{EmptyProjectPath}.filename().string() +
                                                    "')  has a wrong file ending");
  EXPECT_THAT(parser.getError(), StrEq(ErrorMessage));
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, emptyProjectFile) {
  constexpr auto EmptyProjectPath = "/tmp/empty.srctrlprj";
  std::vector<std::string> args{"--project-file", EmptyProjectPath};
  auto fileHandler = FileHandler::createEmptyFile(args.back());

  commandline::CommandLineParser parser({});
  parser.preparse(args);
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_FALSE(parser.exitApplication());
  EXPECT_TRUE(parser.hasError());
  const auto ErrorMessage = utility::decodeFromUtf8("Provided Projectfile is not valid:\n* Provided Projectfile('"s +
                                                    fs::path{EmptyProjectPath}.filename().string() +
                                                    "')  could not be loaded (invalid)");
  EXPECT_THAT(parser.getError(), StrEq(ErrorMessage));
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
  // EXPECT_THAT(parser.getProjectFilePath().str(), StrEq(fs::path {EmptyProjectPath}.filename().string()));
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, loadTutorialProject) {
  constexpr auto ProjectPath = "../app/user/projects/tutorial/tutorial.srctrlprj";
  std::vector<std::string> args{"--project-file", ProjectPath};
  auto fileHandler = FileHandler::createEmptyFile(args.back());

  commandline::CommandLineParser parser({});
  parser.preparse(args);
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_FALSE(parser.exitApplication());
  EXPECT_FALSE(parser.hasError()) << parser.getError();
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
  auto resultPath = fs::absolute(parser.getProjectFilePath().str());
  auto expectedPath = fs::absolute(ProjectPath);
  EXPECT_EQ(resultPath, expectedPath);
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, initStateNoRegisitedCommands) {
  std::vector<std::string> args{"somthing"};
  commandline::CommandLineParser parser({});
  parser.preparse(args);
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_FALSE(parser.exitApplication());
  EXPECT_TRUE(parser.hasError());
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}

// NOLINTNEXTLINE
TEST(CommandLineParserConfig, initStateRegisitedCommands) {
  std::vector<std::string> args{"config --help"};
  commandline::CommandLineParser parser({});
  parser.preparse(args);
  EXPECT_FALSE(parser.runWithoutGUI());
  EXPECT_FALSE(parser.exitApplication());
  EXPECT_TRUE(parser.hasError());
  EXPECT_EQ(RefreshMode::UpdatedFiles, parser.getRefreshMode());
  EXPECT_FALSE(parser.getShallowIndexingRequested());
}
