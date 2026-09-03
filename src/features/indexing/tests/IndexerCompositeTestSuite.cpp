#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "data/storage/IntermediateStorage.h"
#include "indexing/logic/IndexerCommand.h"
#include "indexing/logic/IndexerComposite.h"
#include "indexing/tests/MockedIndexer.hpp"
#include "language_packages.h"

using namespace testing;

struct MockedIndexerCommand final : public IndexerCommand {
  MockedIndexerCommand() : IndexerCommand(FilePath{}) {}
  MOCK_METHOD(IndexerCommandType, getIndexerCommandType, (), (const, override));
};

struct IndexerCompositeFix : testing::Test {};

TEST(IndexerComposite, getSupportedIndexerCommandType) {
  EXPECT_EQ(INDEXER_COMMAND_UNKNOWN, IndexerComposite{}.getSupportedIndexerCommandType());
}

#if BUILD_CXX_LANGUAGE_PACKAGE
TEST_F(IndexerCompositeFix, missingIndexer) {
  IndexerComposite indexerComposite;

  auto mockedIndexerCommand = std::make_shared<MockedIndexerCommand>();
  EXPECT_CALL(*mockedIndexerCommand, getIndexerCommandType).WillOnce(Return(INDEXER_COMMAND_CXX)).WillOnce(Return(INDEXER_COMMAND_CXX));
  EXPECT_FALSE(indexerComposite.index(mockedIndexerCommand));
}
#endif

#if BUILD_CXX_LANGUAGE_PACKAGE
TEST(IndexerComposite, goodCase) {
  IndexerComposite indexerComposite;

  auto mockedIndexerCommand = std::make_shared<MockedIndexerCommand>();
  EXPECT_CALL(*mockedIndexerCommand, getIndexerCommandType).WillOnce(Return(INDEXER_COMMAND_CXX));
  auto mockedIndexer = std::make_shared<MockedIndexer>();
  EXPECT_CALL(*mockedIndexer, getSupportedIndexerCommandType).WillOnce(Return(INDEXER_COMMAND_CXX));
  indexerComposite.addIndexer(mockedIndexer);

  auto intermediateStorage = std::make_shared<IntermediateStorage>();
  EXPECT_CALL(*mockedIndexer, index).WillOnce(Return(intermediateStorage));
  auto result = indexerComposite.index(mockedIndexerCommand);

  EXPECT_CALL(*mockedIndexer, interrupt).WillOnce(Return());
  indexerComposite.interrupt();
}
#endif