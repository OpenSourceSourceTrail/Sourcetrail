#include <gmock/gmock.h>

#include "IndexerBase.h"

struct MockedIndexer : public IndexerBase {
  MOCK_METHOD(IndexerCommandType, getSupportedIndexerCommandType, (), (const, override));
  using IntermediateStoragePtr = std::shared_ptr<IntermediateStorage>;
  using IndexerCommandPtr = std::shared_ptr<IndexerCommand>;
  MOCK_METHOD(IntermediateStoragePtr, index, (IndexerCommandPtr), (override));
  MOCK_METHOD(void, interrupt, (), (override));
};