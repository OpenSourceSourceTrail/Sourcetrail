#include <gtest/gtest.h>

#include "Convert.h"
#include "FilePath.h"
#include "indexing/logic/IndexerCommandJava.h"

TEST(IndexerCommandJava, getStaticIndexerCommandTypeReturnsJava) {
  EXPECT_EQ(INDEXER_COMMAND_JAVA, IndexerCommandJava::getStaticIndexerCommandType());
}

TEST(IndexerCommandJava, roundTripsThroughProto) {
  const FilePath sourceFilePath(L"/project/src/Main.java");
  const std::set<FilePath> classPaths{FilePath(L"/project/lib/a.jar"), FilePath(L"/project/lib/b.jar")};
  const std::wstring languageStandard = L"17";

  const IndexerCommandJava command(sourceFilePath, classPaths, languageStandard);

  const sourcetrail::IndexerCommand msg = proto::convert::toProto(&command);
  EXPECT_EQ(sourcetrail::IndexerCommand::JAVA, msg.type());

  const std::shared_ptr<IndexerCommand> roundTripped = proto::convert::fromProto(msg);
  ASSERT_TRUE(roundTripped);
  EXPECT_EQ(INDEXER_COMMAND_JAVA, roundTripped->getIndexerCommandType());
  EXPECT_EQ(sourceFilePath, roundTripped->getSourceFilePath());

  const auto* javaCommand = dynamic_cast<const IndexerCommandJava*>(roundTripped.get());
  ASSERT_TRUE(javaCommand);
  EXPECT_EQ(classPaths, javaCommand->getClassPaths());
  EXPECT_EQ(languageStandard, javaCommand->getLanguageStandard());
}
