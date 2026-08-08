#pragma once

#include "FilePath.h"

class AppPath final {
public:
  static FilePath getSharedDataDirectoryPath();
  static bool setSharedDataDirectoryPath(const FilePath& path);

  static FilePath getPluginsDirectoryPath();

private:
  static FilePath s_sharedDataDirectoryPath;
};