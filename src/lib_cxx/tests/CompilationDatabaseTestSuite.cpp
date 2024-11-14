#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "CompilationDatabase.h"
#include "ScopedTemporaryFile.hpp"

using namespace testing;

struct MockedLoggerCompilationDatabase : public Test {
public:
};

TEST_F(MockedLoggerCompilationDatabase, emptyPath) {
  const utility::CompilationDatabase compilationDB(FilePath{});
  EXPECT_THAT(compilationDB.getAllHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getSystemHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getFrameworkHeaderPaths(), testing::IsEmpty());
}

TEST_F(MockedLoggerCompilationDatabase, MissingFile) {
  const utility::CompilationDatabase compilationDB(FilePath{"path/not/exists/compile_commands.json"});
  EXPECT_THAT(compilationDB.getAllHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getSystemHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getFrameworkHeaderPaths(), testing::IsEmpty());
}

TEST_F(MockedLoggerCompilationDatabase, InvalidJSON) {
  auto invalidJSON = utility::ScopedTemporaryFile::createFile("InvalidJSON.json", "[{}]");
  ASSERT_TRUE(invalidJSON);

  const utility::CompilationDatabase compilationDB(FilePath{invalidJSON->getFilePath().string()});
  EXPECT_THAT(compilationDB.getAllHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getSystemHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getFrameworkHeaderPaths(), testing::IsEmpty());
}

#ifndef _WIN32
TEST_F(MockedLoggerCompilationDatabase, goodCase) {
  const auto CompilationFilePath = FilePath{"data/SourceGroupTestSuite/cxx_cdb/input/compile_commands.json"};
  const utility::CompilationDatabase compilationDB(CompilationFilePath);
  EXPECT_THAT(compilationDB.getAllHeaderPaths(), testing::Contains(FilePath{"/include/path/from/cdb"}));
  EXPECT_THAT(compilationDB.getHeaderPaths(), testing::IsEmpty());
  EXPECT_THAT(compilationDB.getSystemHeaderPaths(), testing::Contains(FilePath{"/include/path/from/cdb"}));
  EXPECT_THAT(compilationDB.getFrameworkHeaderPaths(), testing::IsEmpty());
  // TODO(Hussein): Missing logging
}
#endif
