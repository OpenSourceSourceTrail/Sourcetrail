#pragma once
#include "FilePath.h"
#include "TimeStamp.h"

struct FileInfo final {
  FileInfo();
  explicit FileInfo(FilePath path);
  FileInfo(FilePath path, TimeStamp lastWriteTime);

  FilePath path;
  TimeStamp lastWriteTime;
};
