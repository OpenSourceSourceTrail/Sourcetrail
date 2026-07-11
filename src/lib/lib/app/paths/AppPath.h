#pragma once

#include "FilePath.h"

class AppPath final {
public:
  static FilePath getSharedDataDirectoryPath();
  static bool setSharedDataDirectoryPath(const FilePath& path);

  static FilePath getCxxIndexerFilePath();
  static bool setCxxIndexerDirectoryPath(const FilePath& path);

private:
  static FilePath s_sharedDataDirectoryPath;
  static FilePath s_cxxIndexerDirectoryPath;
};