#include "NameDelimiterType.h"

#include <vector>

std::wstring nameDelimiterTypeToString(NameDelimiterType delimiter) {
  switch(delimiter) {
  case NAME_DELIMITER_FILE:
    return L"/";
  case NAME_DELIMITER_CXX:
    return L"::";
  default:
    break;
  }
  return L"@";
}

NameDelimiterType stringToNameDelimiterType(const std::wstring& s) {
  if(s == nameDelimiterTypeToString(NAME_DELIMITER_FILE)) {
    return NAME_DELIMITER_FILE;
  }
  if(s == nameDelimiterTypeToString(NAME_DELIMITER_CXX)) {
    return NAME_DELIMITER_CXX;
  }
  return NAME_DELIMITER_UNKNOWN;
}

NameDelimiterType detectDelimiterType(const std::wstring& name) {
  const std::vector<NameDelimiterType> allDelimiters{NAME_DELIMITER_FILE, NAME_DELIMITER_CXX};

  for(NameDelimiterType delimiter : allDelimiters) {
    if(name.find(nameDelimiterTypeToString(delimiter)) != std::wstring::npos) {
      return delimiter;
    }
  }

  return NAME_DELIMITER_UNKNOWN;
}