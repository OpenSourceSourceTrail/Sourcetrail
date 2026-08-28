#include "app/paths/AppPath.h"

#include "utilityApp.h"

FilePath AppPath::s_sharedDataDirectoryPath(L"");

FilePath AppPath::getSharedDataDirectoryPath() {
  return s_sharedDataDirectoryPath;
}

bool AppPath::setSharedDataDirectoryPath(const FilePath& path) {
  if(!path.empty()) {
    s_sharedDataDirectoryPath = path;
    return true;
  }
  return false;
}

FilePath AppPath::getPluginsDirectoryPath() {
  return s_sharedDataDirectoryPath.getConcatenated(L"plugins");
}
