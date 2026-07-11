#include "FileInfo.h"

FileInfo::FileInfo() : path(FilePath(L"")) {}

FileInfo::FileInfo(FilePath path_) : path(std::move(path_)) {}

FileInfo::FileInfo(FilePath path_, TimeStamp lastWriteTime_) : path(std::move(path_)), lastWriteTime(lastWriteTime_) {}
