#pragma once
// STL
#include <string>
// internal
#include "SourceGroupType.h"

enum LanguageType {
  LANGUAGE_CPP,
  LANGUAGE_C,
  LANGUAGE_JAVA,
  LANGUAGE_CUSTOM,
  LANGUAGE_UNKNOWN
};

std::string languageTypeToString(LanguageType type);

LanguageType stringToLanguageType(const std::string& typeString);

LanguageType getLanguageTypeForSourceGroupType(SourceGroupType type);