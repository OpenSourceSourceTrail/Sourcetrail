#include <iostream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "CommandLineParser.h"
#include "IApplicationSettings.hpp"
#include "MockedApplicationSetting.hpp"

struct CommandlineFix : testing::Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mMocked);
  }
  void TearDown() override {
    IApplicationSettings::setInstance(nullptr);
    mMocked.reset();
  }
  std::shared_ptr<testing::StrictMock<MockedApplicationSettings>> mMocked =
      std::make_shared<testing::StrictMock<MockedApplicationSettings>>();
};

TEST_F(CommandlineFix, version) {
  std::vector<std::string> args({"--version", "help"});

  std::stringstream redStream;
  auto oldBuf = std::cout.rdbuf(redStream.rdbuf());

  commandline::CommandLineParser parser("2016.1");
  parser.preparse(args);
  parser.parse();

  std::cout.rdbuf(oldBuf);

  EXPECT_TRUE(redStream.str() == "Sourcetrail Version 2016.1\n");
}

TEST_F(CommandlineFix, commandConfigFilepathVector) {
  std::vector<std::string> args({"config", "-g", "/usr", "-g", "/usr/share/include", "-g", "/opt/test/include"});

  EXPECT_CALL(*mMocked, setHeaderSearchPaths).WillOnce(testing::Return(false));

  commandline::CommandLineParser parser("2");
  parser.preparse(args);
  parser.parse();
}

TEST_F(CommandlineFix, commandConfigFilepathVectorCommaSeparated) {
  std::vector<std::string> args({"config", "--global-header-search-paths", "/usr,/usr/include,/include,/opt/include"});

  EXPECT_CALL(*mMocked, setHeaderSearchPaths).WillOnce(testing::Return(false));

  commandline::CommandLineParser parser("2");
  parser.preparse(args);
  parser.parse();
}

TEST_F(CommandlineFix, commandConfigBoolOptions) {
  std::vector<std::string> args({"config", "--use-processes", "false"});

  EXPECT_CALL(*mMocked, setMultiProcessIndexingEnabled(false)).WillOnce(::testing::Return());

  commandline::CommandLineParser parser("2");
  parser.preparse(args);
  parser.parse();

  EXPECT_CALL(*mMocked, setMultiProcessIndexingEnabled(true)).WillOnce(::testing::Return());

  std::vector<std::string> args1({"config", "--use-processes", "true"});

  parser.preparse(args1);
  parser.parse();
}