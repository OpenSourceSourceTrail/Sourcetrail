#include "SourceLocationCollection.h"
// STL
#include <utility>
// fmt
#include <fmt/format.h>
// internal
#include "logging.h"
#include "SourceLocationFile.h"

SourceLocationCollection::SourceLocationCollection() = default;

SourceLocationCollection::~SourceLocationCollection() = default;

const std::map<FilePath, std::shared_ptr<SourceLocationFile>>& SourceLocationCollection::getSourceLocationFiles() const {
  return m_files;
}

size_t SourceLocationCollection::getSourceLocationCount() const {
  size_t count = 0;
  for(const auto& [file, pSourceLocation] : m_files) {
    count += pSourceLocation->getSourceLocationCount();
  }
  return count;
}

size_t SourceLocationCollection::getSourceLocationFileCount() const {
  return m_files.size();
}

std::shared_ptr<SourceLocationFile> SourceLocationCollection::getSourceLocationFileByPath(const FilePath& filePath) const {
  auto iterator = m_files.find(filePath);
  if(iterator != m_files.end()) {
    return iterator->second;
  }

  return nullptr;
}

SourceLocation* SourceLocationCollection::getSourceLocationById(Id locationId) const {
  for(const auto& pFile : m_files) {
    auto* pLocation = pFile.second->getSourceLocationById(locationId);
    if(pLocation != nullptr) {
      return pLocation;
    }
  }

  return nullptr;
}

SourceLocation* SourceLocationCollection::addSourceLocation(LocationType type,
                                                            Id locationId,
                                                            std::vector<Id> tokenIds,
                                                            const FilePath& filePath,
                                                            size_t startLineNumber,
                                                            size_t startColumnNumber,
                                                            size_t endLineNumber,
                                                            size_t endColumnNumber) {
  if(startLineNumber > endLineNumber || (startLineNumber == endLineNumber && startColumnNumber > endColumnNumber)) {
    LOG_ERROR(fmt::format("SourceLocation has wrong boundaries: {} {}:{} {}:{}",
                          filePath.str(),
                          startLineNumber,
                          startColumnNumber,
                          endLineNumber,
                          endColumnNumber));
    return nullptr;
  }

  SourceLocationFile* file = createSourceLocationFile(filePath);
  if(file->isWhole()) {
    return nullptr;
  }

  return file->addSourceLocation(
      type, locationId, std::move(tokenIds), startLineNumber, startColumnNumber, endLineNumber, endColumnNumber);
}

SourceLocation* SourceLocationCollection::addSourceLocationCopy(const SourceLocation* location) {
  SourceLocationFile* other = location->getSourceLocationFile();
  SourceLocationFile* file = createSourceLocationFile(
      location->getFilePath(), other->getLanguage(), other->isWhole(), other->isComplete(), other->isIndexed());
  return file->addSourceLocationCopy(location);
}

void SourceLocationCollection::addSourceLocationCopies(const SourceLocationCollection* other) {
  other->forEachSourceLocationFile(
      [this](const std::shared_ptr<SourceLocationFile>& otherFile) { addSourceLocationCopies(otherFile.get()); });
}

void SourceLocationCollection::addSourceLocationCopies(const SourceLocationFile* otherFile) {
  SourceLocationFile* file = createSourceLocationFile(
      otherFile->getFilePath(), otherFile->getLanguage(), otherFile->isWhole(), otherFile->isComplete(), otherFile->isIndexed());

  otherFile->forEachSourceLocation([file](SourceLocation* otherLocation) { file->addSourceLocationCopy(otherLocation); });
}

void SourceLocationCollection::addSourceLocationFile(std::shared_ptr<SourceLocationFile> pFile) {
  m_files.emplace(pFile->getFilePath(), pFile);
}

void SourceLocationCollection::forEachSourceLocationFile(const std::function<void(std::shared_ptr<SourceLocationFile>)>& func) const {
  for(const auto& pFile : m_files) {
    func(pFile.second);
  }
}

void SourceLocationCollection::forEachSourceLocation(const std::function<void(SourceLocation*)>& func) const {
  for(const auto& pFile : m_files) {
    pFile.second->forEachSourceLocation(func);
  }
}

SourceLocationFile* SourceLocationCollection::createSourceLocationFile(
    const FilePath& filePath, const std::wstring& language, bool isWhole, bool isComplete, bool isIndexed) {
  auto* pFile = getSourceLocationFileByPath(filePath).get();
  if(pFile != nullptr) {
    return pFile;
  }

  auto pSourceLocationFile = std::make_shared<SourceLocationFile>(filePath, language, isWhole, isComplete, isIndexed);
  m_files.emplace(filePath, pSourceLocationFile);
  return pSourceLocationFile.get();
}

std::wostream& operator<<(std::wostream& ostream, const SourceLocationCollection& base) {
  ostream << L"Locations:\n";
  base.forEachSourceLocationFile(
      [&ostream](const std::shared_ptr<SourceLocationFile>& pSourceLocationFile) { ostream << *(pSourceLocationFile); });
  return ostream;
}