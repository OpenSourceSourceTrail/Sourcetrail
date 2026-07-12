package com.sourcetrail.indexer;

import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;

public interface Indexer {
  IntermediateStorage index(IndexerCommand command);
}
