#pragma once
// internal
#include "Indexer.h"
#include "IndexerCommandCxx.h"

class IndexerCxx final : public Indexer<IndexerCommandCxx> {
private:
  void doIndex(std::shared_ptr<IndexerCommandCxx> indexerCommand,
               std::shared_ptr<ParserClientImpl> parserClient,
               std::shared_ptr<IndexerStateInfo> indexerStateInfo) override;
};
