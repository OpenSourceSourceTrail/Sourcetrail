#pragma once
// STL
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <vector>
// internal
#include "LocationType.h"
#include "GlobalId.hpp"

class FilePath;
class SourceLocation;
class SourceLocationFile;

class SourceLocationCollection final {
public:
  SourceLocationCollection();

  SourceLocationCollection(const SourceLocationCollection&) = delete;
  SourceLocationCollection& operator=(const SourceLocationCollection&) = delete;
  SourceLocationCollection(SourceLocationCollection&&) = delete;
  SourceLocationCollection& operator=(SourceLocationCollection&&) = delete;

  ~SourceLocationCollection();

  [[nodiscard]] const std::map<FilePath, std::shared_ptr<SourceLocationFile>>& getSourceLocationFiles() const;

  [[nodiscard]] size_t getSourceLocationCount() const;

  [[nodiscard]] size_t getSourceLocationFileCount() const;

  [[nodiscard]] std::shared_ptr<SourceLocationFile> getSourceLocationFileByPath(const FilePath& filePath) const;

  [[nodiscard]] SourceLocation* getSourceLocationById(Id locationId) const;

  SourceLocation* addSourceLocation(LocationType type,
                                    Id locationId,
                                    std::vector<Id> tokenIds,
                                    const FilePath& filePath,
                                    size_t startLineNumber,
                                    size_t startColumnNumber,
                                    size_t endLineNumber,
                                    size_t endColumnNumber);

  SourceLocation* addSourceLocationCopy(const SourceLocation* location);

  void addSourceLocationCopies(const SourceLocationCollection* other);

  void addSourceLocationCopies(const SourceLocationFile* otherFile);

  void addSourceLocationFile(std::shared_ptr<SourceLocationFile> file);

  void forEachSourceLocationFile(const std::function<void(std::shared_ptr<SourceLocationFile>)>& func) const;

  void forEachSourceLocation(const std::function<void(SourceLocation*)>& func) const;

private:
  SourceLocationFile* createSourceLocationFile(const FilePath& filePath,
                                               const std::wstring& language = L"",
                                               bool isWhole = false,
                                               bool isComplete = false,
                                               bool isIndexed = false);

  std::map<FilePath, std::shared_ptr<SourceLocationFile>> m_files;
};

std::wostream& operator<<(std::wostream& ostream, const SourceLocationCollection& base);