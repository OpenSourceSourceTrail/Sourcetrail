#include "ParseLocation.h"

ParseLocation::ParseLocation() : fileId(0), startLineNumber(0), startColumnNumber(0), endLineNumber(0), endColumnNumber(0) {}

ParseLocation::ParseLocation(Id fileId_, size_t lineNumber_, size_t columnNumber_)
    : fileId(fileId_)
    , startLineNumber(lineNumber_)
    , startColumnNumber(columnNumber_)
    , endLineNumber(lineNumber_)
    , endColumnNumber(columnNumber_) {}

ParseLocation::ParseLocation(
    Id fileId_, size_t startLineNumber_, size_t startColumnNumber_, size_t endLineNumber_, size_t endColumnNumber_)
    : fileId(fileId_)
    , startLineNumber(startLineNumber_)
    , startColumnNumber(startColumnNumber_)
    , endLineNumber(endLineNumber_)
    , endColumnNumber(endColumnNumber_) {}

bool ParseLocation::isValid() const {
  return fileId;
}
