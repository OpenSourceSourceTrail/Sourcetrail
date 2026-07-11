#include "IndexerJavaStub.h"

#include "IntermediateStorage.h"

IndexerCommandType IndexerJavaStub::getSupportedIndexerCommandType() const {
  return INDEXER_COMMAND_JAVA;
}

std::shared_ptr<IntermediateStorage> IndexerJavaStub::index(std::shared_ptr<IndexerCommand> /*indexerCommand*/) {
  return std::make_shared<IntermediateStorage>();
}

void IndexerJavaStub::interrupt() {}
