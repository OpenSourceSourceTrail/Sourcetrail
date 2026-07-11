#include "SourceLocation.h"
// internal
#include "SourceLocationFile.h"

SourceLocation::SourceLocation(SourceLocationFile* pFile,
                               LocationType type,
                               Id locationId,
                               std::vector<Id> tokenIds,
                               size_t lineNumber,
                               size_t columnNumber,
                               bool isStart)
    : m_file(pFile)
    , m_type(type)
    , m_locationId(locationId)
    , m_tokenIds(std::move(tokenIds))
    , m_lineNumber(lineNumber)
    , m_columnNumber(columnNumber)
    , m_other(nullptr)
    , m_isStart(isStart) {}

SourceLocation::SourceLocation(SourceLocation* pOther, size_t lineNumber, size_t columnNumber)
    : m_file(pOther->m_file)
    , m_type(pOther->m_type)
    , m_locationId(pOther->m_locationId)
    , m_tokenIds(pOther->m_tokenIds)
    , m_lineNumber(lineNumber)
    , m_columnNumber(columnNumber)
    , m_other(pOther)
    , m_isStart(!pOther->m_isStart) {
  pOther->setOtherLocation(this);
}

SourceLocation::Ptr SourceLocation::createEndSourceLocation(SourceLocation* pOther, size_t lineNumber, size_t columnNumber) {
  auto ptr = std::make_shared<SourceLocation>(
      pOther->m_file, pOther->m_type, pOther->m_locationId, pOther->m_tokenIds, lineNumber, columnNumber, !pOther->m_isStart);
  ptr->m_other = pOther;
  return ptr;
}

SourceLocation::SourceLocation(const SourceLocation* pOther, SourceLocationFile* pFile)
    : m_file(pFile)
    , m_type(pOther->m_type)
    , m_locationId(pOther->m_locationId)
    , m_tokenIds(pOther->m_tokenIds)
    , m_lineNumber(pOther->m_lineNumber)
    , m_columnNumber(pOther->m_columnNumber)
    , m_other(nullptr)
    , m_isStart(pOther->m_isStart) {}

SourceLocation::Ptr SourceLocation::createSourceLocationFromAnother(const SourceLocation* pOther, SourceLocationFile* pFile) {
  return std::make_shared<SourceLocation>(
      pFile, pOther->m_type, pOther->m_locationId, pOther->m_tokenIds, pOther->m_lineNumber, pOther->m_columnNumber, pOther->m_isStart);
}

SourceLocation::~SourceLocation() = default;


bool SourceLocation::operator==(const SourceLocation& rhs) const {
  return (getLineNumber() == rhs.getLineNumber() && getColumnNumber() == rhs.getColumnNumber() &&
          getLocationId() == rhs.getLocationId() && getType() == rhs.getType());
}

bool SourceLocation::operator<(const SourceLocation& rhs) const {
  if(getLineNumber() != rhs.getLineNumber()) {
    return getLineNumber() < rhs.getLineNumber();
  }

  if(getColumnNumber() != rhs.getColumnNumber()) {
    return getColumnNumber() < rhs.getColumnNumber();
  }

  return getLocationId() < rhs.getLocationId();
}

bool SourceLocation::operator>(const SourceLocation& rhs) const {
  if(getLineNumber() != rhs.getLineNumber()) {
    return getLineNumber() > rhs.getLineNumber();
  }

  if(getColumnNumber() != rhs.getColumnNumber()) {
    return getColumnNumber() > rhs.getColumnNumber();
  }

  return getLocationId() > rhs.getLocationId();
}

bool SourceLocation::contains(const SourceLocation& other) const {
  const SourceLocation* pStart = getStartLocation();
  const SourceLocation* pOtherStart = other.getStartLocation();

  if(pStart->getLineNumber() > pOtherStart->getLineNumber()) {
    return false;
  }

  if(pStart->getLineNumber() == pOtherStart->getLineNumber() && pStart->getColumnNumber() > pOtherStart->getColumnNumber()) {
    return false;
  }

  const SourceLocation* pEnd = getEndLocation();
  const SourceLocation* pOtherEnd = other.getEndLocation();

  if(pEnd->getLineNumber() < pOtherEnd->getLineNumber()) {
    return false;
  }

  if(pEnd->getLineNumber() == pOtherEnd->getLineNumber() && pEnd->getColumnNumber() < pOtherEnd->getColumnNumber()) {
    return false;
  }

  return true;
}

SourceLocationFile* SourceLocation::getSourceLocationFile() const {
  return m_file;
}

Id SourceLocation::getLocationId() const {
  return m_locationId;
}

const std::vector<Id>& SourceLocation::getTokenIds() const {
  return m_tokenIds;
}

LocationType SourceLocation::getType() const {
  return m_type;
}

size_t SourceLocation::getColumnNumber() const {
  return m_columnNumber;
}

size_t SourceLocation::getLineNumber() const {
  return m_lineNumber;
}

const FilePath& SourceLocation::getFilePath() const {
  return (m_file == nullptr) ? FilePath::EmptyFilePath() : m_file->getFilePath();
}

const SourceLocation* SourceLocation::getOtherLocation() const {
  return m_other;
}

void SourceLocation::setOtherLocation(SourceLocation* other) {
  m_other = other;
}

const SourceLocation* SourceLocation::getStartLocation() const {
  return m_isStart ? this : m_other;
}

const SourceLocation* SourceLocation::getEndLocation() const {
  return m_isStart ? m_other : this;
}

bool SourceLocation::isStartLocation() const {
  return m_isStart;
}

bool SourceLocation::isEndLocation() const {
  return !m_isStart;
}

bool SourceLocation::isScopeLocation() const {
  return m_type == LOCATION_SCOPE;
}

bool SourceLocation::isFullTextSearchMatch() const {
  return m_type == LOCATION_FULLTEXT_SEARCH;
}

std::wostream& operator<<(std::wostream& ostream, const SourceLocation& location) {
  if(location.isStartLocation()) {
    ostream << L'<';
  }

  ostream << location.getColumnNumber() << L":[ ";
  for(Id tokenId : location.getTokenIds()) {
    ostream << L'\b' << tokenId << L' ';
  }
  ostream << L"\b]";

  if(location.isEndLocation()) {
    ostream << L'>';
  }

  ostream << L' ';
  return ostream;
}