#include "ResourcePaths.h"

#include "AppPath.h"
#include "utilityApp.h"

FilePath ResourcePaths::getColorSchemesDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/color_schemes/");
}

FilePath ResourcePaths::getSyntaxHighlightingRulesDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/syntax_highlighting_rules/");
}

FilePath ResourcePaths::getFallbackDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/fallback/");
}

FilePath ResourcePaths::getFontsDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/fonts/");
}

FilePath ResourcePaths::getGuiDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/gui/");
}

FilePath ResourcePaths::getLicenseDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/license/");
}

FilePath ResourcePaths::getCxxCompilerHeaderDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/cxx/include/").getCanonical();
}