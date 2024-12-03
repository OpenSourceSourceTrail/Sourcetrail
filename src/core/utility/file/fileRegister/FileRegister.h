#pragma once
#include <set>
#include <string>

#include "UnorderedCache.h"

class FilePath;
class FilePathFilter;

class FileRegister {
public:
  FileRegister(const FilePath& currentPath, const std::set<FilePath>& indexedPaths, const std::set<FilePathFilter>& excludeFilters);

  virtual ~FileRegister();

  virtual bool hasFilePath(const FilePath& filePath) const;

private:
  const FilePath& m_currentPath;
  const std::set<FilePath> m_indexedPaths;
  const std::set<FilePathFilter> m_excludeFilters;
  mutable UnorderedCache<std::wstring, bool> m_hasFilePathCache;
};