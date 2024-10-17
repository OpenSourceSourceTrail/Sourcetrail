#pragma once

#include <string>

#include "language_packages.h"

enum SourceGroupType {
#if BUILD_CXX_LANGUAGE_PACKAGE
  SOURCE_GROUP_C_EMPTY,
  SOURCE_GROUP_CPP_EMPTY,
  SOURCE_GROUP_CXX_CDB,
  SOURCE_GROUP_CXX_VS,
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  SOURCE_GROUP_CUSTOM_COMMAND,
  SOURCE_GROUP_UNKNOWN
};

std::string sourceGroupTypeToString(SourceGroupType v);

std::string sourceGroupTypeToProjectSetupString(SourceGroupType v);

SourceGroupType stringToSourceGroupType(const std::string& v);