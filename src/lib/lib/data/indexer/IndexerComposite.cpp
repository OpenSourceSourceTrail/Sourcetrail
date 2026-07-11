#include "IndexerComposite.h"

#include "IndexerCommand.h"
#include "IntermediateStorage.h"
#include "logging.h"

IndexerComposite::~IndexerComposite() = default;

IndexerCommandType IndexerComposite::getSupportedIndexerCommandType() const {
  return INDEXER_COMMAND_UNKNOWN;
}

void IndexerComposite::addIndexer(std::shared_ptr<IndexerBase> indexer) {
  m_indexers.emplace(indexer->getSupportedIndexerCommandType(), indexer);
}

std::shared_ptr<IntermediateStorage> IndexerComposite::index(std::shared_ptr<IndexerCommand> indexerCommand) {
  auto iterator = m_indexers.find(indexerCommand->getIndexerCommandType());
  if(iterator != m_indexers.end()) {
    return iterator->second->index(indexerCommand);
  }

  LOG_ERROR("No indexer found that supports \"" + indexerCommandTypeToString(indexerCommand->getIndexerCommandType()) + "\".");
  return {};
}

void IndexerComposite::interrupt() {
  for(auto& [type, indexer] : m_indexers) {
    indexer->interrupt();
  }
}
