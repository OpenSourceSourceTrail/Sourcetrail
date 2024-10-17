#include "LanguageType.h"

std::string languageTypeToString(LanguageType type) {
  switch(type) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  case LANGUAGE_C:
    return "C";
  case LANGUAGE_CPP:
    return "C++";
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  case LANGUAGE_CUSTOM:
    return "Custom";
  case LANGUAGE_UNKNOWN:
    break;
  }
  return "unknown";
}

LanguageType stringToLanguageType(const std::string& typeString) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  if(typeString == languageTypeToString(LANGUAGE_C)) {
    return LANGUAGE_C;
  }
  if(typeString == languageTypeToString(LANGUAGE_CPP)) {
    return LANGUAGE_CPP;
  }
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  if(typeString == languageTypeToString(LANGUAGE_CUSTOM)) {
    return LANGUAGE_CUSTOM;
  }
  return LANGUAGE_UNKNOWN;
}

LanguageType getLanguageTypeForSourceGroupType(SourceGroupType type) {
  switch(type) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  case SOURCE_GROUP_C_EMPTY:
    return LANGUAGE_C;
  case SOURCE_GROUP_CPP_EMPTY:
    return LANGUAGE_CPP;
  case SOURCE_GROUP_CXX_CDB:
    return LANGUAGE_CPP;
  case SOURCE_GROUP_CXX_VS:
    return LANGUAGE_CPP;
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  case SOURCE_GROUP_CUSTOM_COMMAND:
    return LANGUAGE_CUSTOM;
  default:
    break;
  }

  return LANGUAGE_UNKNOWN;
}