#pragma once
// internal
#include "data/indexer/Indexer.h"
#include "data/indexer/IndexerCommandCxx.h"

class IndexerCxx final : public Indexer<IndexerCommandCxx> {
private:
  void doIndex(std::shared_ptr<IndexerCommandCxx> indexerCommand,
               std::shared_ptr<ParserClientImpl> parserClient,
               std::shared_ptr<IndexerStateInfo> indexerStateInfo) override;
};
