#pragma once

#include <string>

enum NameDelimiterType {
  NAME_DELIMITER_UNKNOWN,
  NAME_DELIMITER_FILE,
  NAME_DELIMITER_CXX,
  NAME_DELIMITER_JAVA,
};

std::wstring nameDelimiterTypeToString(NameDelimiterType delimiter);

NameDelimiterType stringToNameDelimiterType(const std::wstring& s);

NameDelimiterType detectDelimiterType(const std::wstring& name);