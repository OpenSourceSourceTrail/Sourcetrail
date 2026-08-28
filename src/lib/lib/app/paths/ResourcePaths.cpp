#include "app/paths/ResourcePaths.h"

#include "app/paths/AppPath.h"
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
  return FilePath{L":/data/fonts/"};
}

FilePath ResourcePaths::getGuiDirectoryPath() {
  return FilePath{L":/data/gui/"};
}

FilePath ResourcePaths::getLicenseDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/license/");
}

FilePath ResourcePaths::getCxxCompilerHeaderDirectoryPath() {
  return AppPath::getSharedDataDirectoryPath().concatenate(L"data/cxx/include/").getCanonical();
}