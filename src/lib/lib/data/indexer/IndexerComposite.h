#pragma once

#include <map>
#include <memory>

#include "IndexerBase.h"

class IndexerComposite final : public IndexerBase {
public:
  ~IndexerComposite() override;

  [[nodiscard]] IndexerCommandType getSupportedIndexerCommandType() const override;

  void addIndexer(std::shared_ptr<IndexerBase> indexer);

  std::shared_ptr<IntermediateStorage> index(std::shared_ptr<IndexerCommand> indexerCommand) override;

  void interrupt() override;

private:
  std::map<IndexerCommandType, std::shared_ptr<IndexerBase>> m_indexers;
};