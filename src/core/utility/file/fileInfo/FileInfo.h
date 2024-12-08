#pragma once
#include "FilePath.h"
#include "TimeStamp.h"

struct FileInfo final {
  FileInfo();
  explicit FileInfo(FilePath path);
  FileInfo(FilePath path, TimeStamp lastWriteTime);
  [[nodiscard]] bool operator==(const FileInfo& rhs) const noexcept {
    return this->path == rhs.path && this->lastWriteTime == rhs.lastWriteTime;
  }
  FilePath path;
  TimeStamp lastWriteTime;
};
