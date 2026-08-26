#pragma once

#include <string>

enum SourceGroupType {
  SOURCE_GROUP_C_EMPTY,
  SOURCE_GROUP_CPP_EMPTY,
  SOURCE_GROUP_CXX_CDB,
  SOURCE_GROUP_CXX_VS,
  SOURCE_GROUP_JAVA_EMPTY,
  SOURCE_GROUP_CUSTOM_COMMAND,
  SOURCE_GROUP_UNKNOWN
};

std::string sourceGroupTypeToString(SourceGroupType v);

std::string sourceGroupTypeToProjectSetupString(SourceGroupType v);

SourceGroupType stringToSourceGroupType(const std::string& v);