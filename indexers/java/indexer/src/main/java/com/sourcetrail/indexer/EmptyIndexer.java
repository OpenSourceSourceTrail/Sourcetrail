package com.sourcetrail.indexer;

import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;

/** Stand-in indexer for increment 3: proves the gRPC round trip works from a real JVM process. */
public final class EmptyIndexer implements Indexer {
  @Override
  public IntermediateStorage index(IndexerCommand command) {
    return IntermediateStorage.newBuilder().setNextId(1).build();
  }
}
