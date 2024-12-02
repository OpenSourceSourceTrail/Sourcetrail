#pragma once
#include <set>
#include <string>
#include <unordered_map>

#include "FilePath.h"

class FileTree final {
public:
  FileTree(const FilePath& rootPath);

  FilePath getAbsoluteRootPathForRelativeFilePath(const FilePath& relativeFilePath);
  std::vector<FilePath> getAbsoluteRootPathsForRelativeFilePath(const FilePath& relativeFilePath);

private:
  std::vector<FilePath> doGetAbsoluteRootPathsForRelativeFilePath(const FilePath& relativeFilePath, bool allowMultipleResults);

  FilePath m_rootPath;
  std::unordered_map<std::wstring, std::set<FilePath>> m_files;
};