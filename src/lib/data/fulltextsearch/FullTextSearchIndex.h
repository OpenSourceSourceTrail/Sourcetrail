#pragma once
// STL
#include <mutex>
#include <vector>

#include <unordered_map>
// internal
#include "SuffixArray.h"
#include "GlobalId.hpp"

class StorageAccess;

// contains all fulltextsearch results of one file
struct FullTextSearchResult {
  Id fileId;
  std::vector<int> positions;
};

struct FullTextSearchFile {
  FullTextSearchFile(Id fileId_, SuffixArray array_) : fileId(fileId_), array(array_) {}
  Id fileId;
  SuffixArray array;
};

class FullTextSearchIndex {
public:
  void addFile(Id fileId, const std::wstring& file);
  std::vector<FullTextSearchResult> searchForTerm(const std::wstring& term) const;

  size_t fileCount() const;

  void clear();

private:
  mutable std::mutex m_filesMutex;
  std::vector<FullTextSearchFile> m_files;
};
