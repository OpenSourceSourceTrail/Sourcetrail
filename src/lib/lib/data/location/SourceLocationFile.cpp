#include "SourceLocationFile.h"

#include <utility>

#include "utilityString.h"

SourceLocationFile::SourceLocationFile(FilePath filePath, std::wstring language, bool isWhole, bool isComplete, bool isIndexed)
    : m_filePath(std::move(filePath))
    , m_language(std::move(language))
    , m_isWhole(isWhole)
    , m_isComplete(isComplete)
    , m_isIndexed(isIndexed) {}

SourceLocationFile::~SourceLocationFile() = default;

const FilePath& SourceLocationFile::getFilePath() const {
  return m_filePath;
}

void SourceLocationFile::setLanguage(const std::wstring& language) {
  m_language = language;
}

const std::wstring& SourceLocationFile::getLanguage() const {
  return m_language;
}

void SourceLocationFile::setIsWhole(bool isWhole) {
  m_isWhole = isWhole;
}

bool SourceLocationFile::isWhole() const {
  return m_isWhole;
}

void SourceLocationFile::setIsComplete(bool isComplete) {
  m_isComplete = isComplete;
}

bool SourceLocationFile::isComplete() const {
  return m_isComplete;
}

void SourceLocationFile::setIsIndexed(bool isIndexed) {
  m_isIndexed = isIndexed;
}

bool SourceLocationFile::isIndexed() const {
  return m_isIndexed;
}

const std::multiset<std::shared_ptr<SourceLocation>, SourceLocationFile::LocationComp>& SourceLocationFile::getSourceLocations() const {
  return m_locations;
}

size_t SourceLocationFile::getSourceLocationCount() const {
  return m_locationIndex.size();
}

size_t SourceLocationFile::getUnscopedStartLocationCount() const {
  return static_cast<size_t>(std::count_if(std::cbegin(m_locations), std::cend(m_locations), [](const auto& pLocation) {
    return pLocation->isStartLocation() && !pLocation->isScopeLocation();
  }));
}

SourceLocation* SourceLocationFile::addSourceLocation(LocationType type,
                                                      Id locationId,
                                                      std::vector<Id> tokenIds,
                                                      size_t startLineNumber,
                                                      size_t startColumnNumber,
                                                      size_t endLineNumber,
                                                      size_t endColumnNumber) {
  auto pStart = std::make_shared<SourceLocation>(this, type, locationId, tokenIds, startLineNumber, startColumnNumber, true);
  auto pEnd = std::make_shared<SourceLocation>(pStart.get(), endLineNumber, endColumnNumber);

  m_locations.insert(pStart);
  m_locations.insert(pEnd);

  if(auto startId = pStart->getLocationId(); startId != 0U) {
    m_locationIndex.emplace(startId, pStart.get());
  }

  return pStart.get();
}

SourceLocation* SourceLocationFile::addSourceLocationCopy(const SourceLocation* location) {
  // Check whether this location was already added or if the other SourceLocation was added.
  auto* pOldLocation = getSourceLocationById(location->getLocationId());
  if(pOldLocation != nullptr) {
    if(pOldLocation->isStartLocation() == location->isStartLocation()) {
      return pOldLocation;
    }

    const auto* pOtherOldLocation = pOldLocation->getOtherLocation();
    if((pOtherOldLocation != nullptr) && pOtherOldLocation->isStartLocation() == location->isStartLocation()) {
      return const_cast<SourceLocation*>(pOtherOldLocation);
    }
  }

  auto pCopy = std::make_shared<SourceLocation>(location, this);
  m_locations.insert(pCopy);

  if(pCopy->getLocationId() != 0U) {
    m_locationIndex.emplace(pCopy->getLocationId(), pCopy.get());
  }

  // If the old location was added before, then link them with each other.
  if(pOldLocation != nullptr) {
    pOldLocation->setOtherLocation(pCopy.get());
    pCopy->setOtherLocation(pOldLocation);
  }

  return pCopy.get();
}

void SourceLocationFile::copySourceLocations(const std::shared_ptr<SourceLocationFile>& pFile) {
  pFile->forEachSourceLocation([this](SourceLocation* pLocation) { addSourceLocationCopy(pLocation); });
}

SourceLocation* SourceLocationFile::getSourceLocationById(Id locationId) const {
  if(auto iterator = m_locationIndex.find(locationId); iterator != m_locationIndex.end()) {
    return iterator->second;
  }

  return nullptr;
}

void SourceLocationFile::forEachSourceLocation(const std::function<void(SourceLocation*)>& func) const {
  std::for_each(std::cbegin(m_locations), std::cend(m_locations), [&func](const auto& location) { func(location.get()); });
}

void SourceLocationFile::forEachStartSourceLocation(const std::function<void(SourceLocation*)>& func) const {
  for(const auto& pLocation : m_locations) {
    if(pLocation->isStartLocation()) {
      func(pLocation.get());
    }
  }
}

void SourceLocationFile::forEachEndSourceLocation(const std::function<void(SourceLocation*)>& func) const {
  for(const auto& pLocation : m_locations) {
    if(pLocation->isEndLocation()) {
      func(pLocation.get());
    }
  }
}

std::shared_ptr<SourceLocationFile> SourceLocationFile::getFilteredByLines(size_t firstLineNumber, size_t lastLineNumber) const {
  auto pOutput = std::make_shared<SourceLocationFile>(getFilePath(), getLanguage(), false, isComplete(), isIndexed());

  for(const auto& pLocation : m_locations) {
    if(pLocation->getLineNumber() >= firstLineNumber && pLocation->getLineNumber() <= lastLineNumber) {
      pOutput->addSourceLocationCopy(pLocation.get());
    }
  }

  return pOutput;
}

std::shared_ptr<SourceLocationFile> SourceLocationFile::getFilteredByType(LocationType type) const {
  std::shared_ptr<SourceLocationFile> ret = std::make_shared<SourceLocationFile>(
      getFilePath(), getLanguage(), false, isComplete(), isIndexed());

  for(const std::shared_ptr<SourceLocation>& location : m_locations) {
    if(location->getType() == type) {
      ret->addSourceLocationCopy(location.get());
    }
  }

  return ret;
}

std::shared_ptr<SourceLocationFile> SourceLocationFile::getFilteredByTypes(const std::vector<LocationType>& types) const {
  size_t typeMask = 0;
  for(LocationType type : types) {
    typeMask |= static_cast<size_t>(1) << type;
  }

  auto pSourceLocationFile = std::make_shared<SourceLocationFile>(
      getFilePath(), getLanguage(), isWhole(), isComplete(), isIndexed());

  for(const auto& pLocation : m_locations) {
    if((static_cast<size_t>(1) << pLocation->getType()) & typeMask) {
      pSourceLocationFile->addSourceLocationCopy(pLocation.get());
    }
  }

  return pSourceLocationFile;
}

std::wostream& operator<<(std::wostream& ostream, const SourceLocationFile& file) {
  ostream << L"file \"" << file.getFilePath().wstr() << L"\"";

  if(file.isWhole()) {
    ostream << L" whole";
  }

  if(file.isComplete()) {
    ostream << L" complete";
  }

  if(file.isIndexed()) {
    ostream << L" indexed";
  }

  size_t line = 0;
  file.forEachSourceLocation([&ostream, &line](SourceLocation* location) {
    if(location->getLineNumber() != line) {
      while(line < location->getLineNumber()) {
        if(line == 0U) {
          line = location->getLineNumber();
        } else {
          line++;
        }

        ostream << L'\n' << line;
      }

      ostream << L":  ";
    }

    ostream << *location;
  });

  ostream << L'\n';
  return ostream;
}