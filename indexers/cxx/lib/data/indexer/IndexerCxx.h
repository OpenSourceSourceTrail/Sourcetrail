#pragma once
// internal
#include "indexing/logic/Indexer.h"
#include "indexing/logic/IndexerCommandCxx.h"

class IndexerCxx final : public Indexer<IndexerCommandCxx> {
private:
  void doIndex(std::shared_ptr<IndexerCommandCxx> indexerCommand,
               std::shared_ptr<ParserClientImpl> parserClient,
               std::shared_ptr<IndexerStateInfo> indexerStateInfo) override;
};
