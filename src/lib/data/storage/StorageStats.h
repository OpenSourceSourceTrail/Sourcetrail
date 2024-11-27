#pragma once

#include "TimeStamp.h"

struct StorageStats final {
  size_t nodeCount = 0;
  size_t edgeCount = 0;

  size_t fileCount = 0;
  size_t completedFileCount = 0;
  size_t fileLOCCount = 0;

  TimeStamp timestamp;
};