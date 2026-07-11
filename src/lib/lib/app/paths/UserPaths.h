#pragma once

#include "FilePath.h"

class UserPaths final {
public:
  static FilePath getUserDataDirectoryPath();
  static void setUserDataDirectoryPath(const FilePath& path);

  static FilePath getAppSettingsFilePath();
  static FilePath getWindowSettingsFilePath();
  static FilePath getLogDirectoryPath();

private:
  static FilePath s_userDataDirectoryPath;
};