#include "SourceGroupType.h"

std::string sourceGroupTypeToString(SourceGroupType v) {
  switch(v) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  case SOURCE_GROUP_C_EMPTY:
    return "C Source Group";
  case SOURCE_GROUP_CPP_EMPTY:
    return "C++ Source Group";
  case SOURCE_GROUP_CXX_CDB:
    return "C/C++ from Compilation Database";
  case SOURCE_GROUP_CXX_VS:
    return "C/C++ from Visual Studio";
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  case SOURCE_GROUP_CUSTOM_COMMAND:
    return "Custom Command Source Group";
  case SOURCE_GROUP_UNKNOWN:
    break;
  }
  return "unknown";
}

std::string sourceGroupTypeToProjectSetupString(SourceGroupType v) {
  switch(v) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  case SOURCE_GROUP_C_EMPTY:
    return "Empty C Source Group";
  case SOURCE_GROUP_CPP_EMPTY:
    return "Empty C++ Source Group";
  case SOURCE_GROUP_CXX_CDB:
    return "C/C++ from Compilation Database";
  case SOURCE_GROUP_CXX_VS:
    return "C/C++ from Visual Studio";
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  case SOURCE_GROUP_CUSTOM_COMMAND:
    return "Custom Command Source Group";
  case SOURCE_GROUP_UNKNOWN:
    break;
  }
  return "unknown";
}

SourceGroupType stringToSourceGroupType(const std::string& v) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  if(v == sourceGroupTypeToString(SOURCE_GROUP_C_EMPTY)) {
    return SOURCE_GROUP_C_EMPTY;
  }
  if(v == sourceGroupTypeToString(SOURCE_GROUP_CPP_EMPTY)) {
    return SOURCE_GROUP_CPP_EMPTY;
  }
  if(v == sourceGroupTypeToString(SOURCE_GROUP_CXX_CDB)) {
    return SOURCE_GROUP_CXX_CDB;
  }
  if(v == sourceGroupTypeToString(SOURCE_GROUP_CXX_VS)) {
    return SOURCE_GROUP_CXX_VS;
  }
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  if(v == sourceGroupTypeToString(SOURCE_GROUP_CUSTOM_COMMAND)) {
    return SOURCE_GROUP_CUSTOM_COMMAND;
  }

  return SOURCE_GROUP_UNKNOWN;
}