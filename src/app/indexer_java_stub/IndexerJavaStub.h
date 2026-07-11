#pragma once

#include "IndexerBase.h"

// Minimal stand-in for the real Java worker (a pure-JVM gRPC client, see docs/plan). Proves the
// plugin discovery -> process launch -> gRPC PullCommand/PushIntermediateStorage round trip for
// INDEXER_COMMAND_JAVA without any real Java parsing.
class IndexerJavaStub final : public IndexerBase {
public:
  [[nodiscard]] IndexerCommandType getSupportedIndexerCommandType() const override;
  std::shared_ptr<IntermediateStorage> index(std::shared_ptr<IndexerCommand> indexerCommand) override;
  void interrupt() override;
};
