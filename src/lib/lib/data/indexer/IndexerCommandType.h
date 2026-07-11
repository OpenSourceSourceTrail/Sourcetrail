#pragma once

#include <string>

#include "language_packages.h"

enum IndexerCommandType {
  INDEXER_COMMAND_UNKNOWN,
#if BUILD_CXX_LANGUAGE_PACKAGE
  INDEXER_COMMAND_CXX,
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  INDEXER_COMMAND_CUSTOM
};

std::string indexerCommandTypeToString(IndexerCommandType type);
IndexerCommandType stringToIndexerCommandType(const std::string& s);