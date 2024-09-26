#pragma once
#include <gmock/gmock.h>

#include "IApplicationSettings.hpp"

struct MockedApplicationSettings : IApplicationSettings {
  MOCK_METHOD(bool, load, (const FilePath&, bool), (override));

  MOCK_METHOD(size_t, getMaxRecentProjectsCount, (), (const, noexcept, override));

  // application
  MOCK_METHOD(std::string, getFontName, (), (const, noexcept, override));
  MOCK_METHOD(void, setFontName, (const std::string& fontName), (noexcept, override));

  MOCK_METHOD(int, getFontSize, (), (const, noexcept, override));
  MOCK_METHOD(void, setFontSize, (int), (noexcept, override));

  MOCK_METHOD(std::string, getTextEncoding, (), (const, noexcept, override));
  MOCK_METHOD(void, setTextEncoding, (const std::string&), (noexcept, override));

  MOCK_METHOD(std::wstring, getColorSchemeName, (), (const, noexcept, override));
  MOCK_METHOD(std::filesystem::path, getColorSchemePath, (), (const, noexcept, override));
  MOCK_METHOD(void, setColorSchemeName, (const std::wstring& colorSchemeName), (noexcept, override));

  MOCK_METHOD(int, getFontSizeMax, (), (const, noexcept, override));
  MOCK_METHOD(void, setFontSizeMax, (int), (noexcept, override));

  MOCK_METHOD(int, getFontSizeMin, (), (const, noexcept, override));
  MOCK_METHOD(void, setFontSizeMin, (int), (noexcept, override));

  MOCK_METHOD(int, getFontSizeStd, (), (const, noexcept, override));
  MOCK_METHOD(void, setFontSizeStd, (int), (noexcept, override));

  MOCK_METHOD(bool, getUseAnimations, (), (const, noexcept, override));
  MOCK_METHOD(void, setUseAnimations, (bool), (noexcept, override));

  MOCK_METHOD(bool, getShowBuiltinTypesInGraph, (), (const, noexcept, override));
  MOCK_METHOD(void, setShowBuiltinTypesInGraph, (bool), (noexcept, override));

  MOCK_METHOD(bool, getShowDirectoryInCodeFileTitle, (), (const, noexcept, override));
  MOCK_METHOD(void, setShowDirectoryInCodeFileTitle, (bool), (noexcept, override));

  MOCK_METHOD(int, getWindowBaseWidth, (), (const, noexcept, override));
  MOCK_METHOD(int, getWindowBaseHeight, (), (const, noexcept, override));

  MOCK_METHOD(float, getScrollSpeed, (), (const, noexcept, override));
  MOCK_METHOD(void, setScrollSpeed, (float), (noexcept, override));

  MOCK_METHOD(bool, getGraphControlsVisible, (), (const, noexcept, override));
  MOCK_METHOD(void, setGraphControlsVisible, (bool), (noexcept, override));

  MOCK_METHOD(GroupType, getGraphGrouping, (), (const, noexcept, override));
  MOCK_METHOD(void, setGraphGrouping, (GroupType), (noexcept, override));

  // screen
  MOCK_METHOD(int, getScreenAutoScaling, (), (const, noexcept, override));
  MOCK_METHOD(void, setScreenAutoScaling, (int), (noexcept, override));

  MOCK_METHOD(float, getScreenScaleFactor, (), (const, noexcept, override));
  MOCK_METHOD(void, setScreenScaleFactor, (float), (noexcept, override));

  // logging
  MOCK_METHOD(bool, getLoggingEnabled, (), (const, noexcept, override));
  MOCK_METHOD(void, setLoggingEnabled, (bool), (noexcept, override));

  MOCK_METHOD(bool, getVerboseIndexerLoggingEnabled, (), (const, noexcept, override));
  MOCK_METHOD(void, setVerboseIndexerLoggingEnabled, (bool), (noexcept, override));

  MOCK_METHOD(std::filesystem::path, getLogDirectoryPath, (), (const, noexcept, override));
  MOCK_METHOD(void, setLogDirectoryPath, (const std::filesystem::path&), (noexcept, override));

  MOCK_METHOD(int, getLogFilter, (), (const, noexcept, override));
  MOCK_METHOD(void, setLogFilter, (int), (noexcept, override));

  MOCK_METHOD(int, getStatusFilter, (), (const, noexcept, override));
  MOCK_METHOD(void, setStatusFilter, (int), (noexcept, override));

  // indexing
  MOCK_METHOD(int, getIndexerThreadCount, (), (const, noexcept, override));
  MOCK_METHOD(void, setIndexerThreadCount, (int), (noexcept, override));

  MOCK_METHOD(bool, getMultiProcessIndexingEnabled, (), (const, noexcept, override));
  MOCK_METHOD(void, setMultiProcessIndexingEnabled, (bool), (noexcept, override));

  MOCK_METHOD(std::vector<std::filesystem::path>, getHeaderSearchPaths, (), (const, noexcept, override));
  MOCK_METHOD(std::vector<std::filesystem::path>, getHeaderSearchPathsExpanded, (), (const, noexcept, override));
  MOCK_METHOD(bool, setHeaderSearchPaths, (const std::vector<std::filesystem::path>&), (noexcept, override));

  MOCK_METHOD(bool, getHasPrefilledHeaderSearchPaths, (), (const, noexcept, override));
  MOCK_METHOD(void, setHasPrefilledHeaderSearchPaths, (bool), (noexcept, override));

  MOCK_METHOD(std::vector<std::filesystem::path>, getFrameworkSearchPaths, (), (const, noexcept, override));
  MOCK_METHOD(std::vector<std::filesystem::path>, getFrameworkSearchPathsExpanded, (), (const, noexcept, override));
  MOCK_METHOD(bool, setFrameworkSearchPaths, (const std::vector<std::filesystem::path>&), (noexcept, override));

  MOCK_METHOD(bool, getHasPrefilledFrameworkSearchPaths, (), (const, noexcept, override));
  MOCK_METHOD(void, setHasPrefilledFrameworkSearchPaths, (bool), (noexcept, override));

  // code
  MOCK_METHOD(int, getCodeTabWidth, (), (const, noexcept, override));
  MOCK_METHOD(void, setCodeTabWidth, (int), (noexcept, override));

  MOCK_METHOD(int, getCodeSnippetSnapRange, (), (const, noexcept, override));
  MOCK_METHOD(void, setCodeSnippetSnapRange, (int), (noexcept, override));

  MOCK_METHOD(int, getCodeSnippetExpandRange, (), (const, noexcept, override));
  MOCK_METHOD(void, setCodeSnippetExpandRange, (int), (noexcept, override));

  MOCK_METHOD(bool, getCodeViewModeSingle, (), (const, noexcept, override));
  MOCK_METHOD(void, setCodeViewModeSingle, (bool), (noexcept, override));

  // user
  MOCK_METHOD(std::vector<std::filesystem::path>, getRecentProjects, (), (const, noexcept, override));
  MOCK_METHOD(bool, setRecentProjects, (const std::vector<std::filesystem::path>&), (noexcept, override));

  MOCK_METHOD(bool, getSeenErrorHelpMessage, (), (const, noexcept, override));
  MOCK_METHOD(void, setSeenErrorHelpMessage, (bool), (noexcept, override));

  MOCK_METHOD(std::filesystem::path, getLastFilepickerLocation, (), (const, noexcept, override));
  MOCK_METHOD(void, setLastFilepickerLocation, (const std::filesystem::path&), (noexcept, override));

  MOCK_METHOD(float, getGraphZoomLevel, (), (const, noexcept, override));
  MOCK_METHOD(void, setGraphZoomLevel, (float), (noexcept, override));

  // network
  MOCK_METHOD(int, getPluginPort, (), (const, noexcept, override));
  MOCK_METHOD(void, setPluginPort, (int), (noexcept, override));

  MOCK_METHOD(int, getSourcetrailPort, (), (const, noexcept, override));
  MOCK_METHOD(void, setSourcetrailPort, (int), (noexcept, override));

  // controls
  MOCK_METHOD(int, getControlsMouseBackButton, (), (const, noexcept, override));
  MOCK_METHOD(int, getControlsMouseForwardButton, (), (const, noexcept, override));

  MOCK_METHOD(bool, getControlsGraphZoomOnMouseWheel, (), (const, noexcept, override));
  MOCK_METHOD(void, setControlsGraphZoomOnMouseWheel, (bool), (noexcept, override));
};
