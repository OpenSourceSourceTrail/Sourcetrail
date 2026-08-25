#pragma once

#include <string>

enum IndexerCommandType {
  INDEXER_COMMAND_UNKNOWN,
  INDEXER_COMMAND_CXX,
  INDEXER_COMMAND_JAVA,
  INDEXER_COMMAND_CUSTOM
};

std::string indexerCommandTypeToString(IndexerCommandType type);
IndexerCommandType stringToIndexerCommandType(const std::string& s);