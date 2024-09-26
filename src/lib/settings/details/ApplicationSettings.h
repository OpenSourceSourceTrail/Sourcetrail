#pragma once
#include <cassert>
#include <filesystem>

#include "GroupType.h"
#include "IApplicationSettings.hpp"

class TimeStamp;
class Version;

class ApplicationSettings : public IApplicationSettings {
public:
  static const size_t VERSION;

  ApplicationSettings();

  ApplicationSettings(const ApplicationSettings&) = delete;
  ApplicationSettings(ApplicationSettings&&) = delete;
  ApplicationSettings& operator=(const ApplicationSettings&) = delete;
  ApplicationSettings& operator=(ApplicationSettings&&) = delete;

  ~ApplicationSettings() override;

  bool load(const FilePath& filePath, bool readOnly = false) override;

  [[nodiscard]] size_t getMaxRecentProjectsCount() const noexcept override;

  // application
  [[nodiscard]] std::string getFontName() const noexcept override;
  void setFontName(const std::string& fontName) noexcept override;

  int getFontSize() const noexcept override;
  void setFontSize(int fontSize) noexcept override;

  [[nodiscard]] std::string getTextEncoding() const noexcept override;
  void setTextEncoding(const std::string& textEncoding) noexcept override;

  [[nodiscard]] std::wstring getColorSchemeName() const noexcept override;
  std::filesystem::path getColorSchemePath() const noexcept override;
  void setColorSchemeName(const std::wstring& colorSchemeName) noexcept override;

  int getFontSizeMax() const noexcept override;
  void setFontSizeMax(int fontSizeMax) noexcept override;

  int getFontSizeMin() const noexcept override;
  void setFontSizeMin(int fontSizeMin) noexcept override;

  int getFontSizeStd() const noexcept override;
  void setFontSizeStd(int fontSizeStd) noexcept override;

  bool getUseAnimations() const noexcept override;
  void setUseAnimations(bool useAnimations) noexcept override;

  bool getShowBuiltinTypesInGraph() const noexcept override;
  void setShowBuiltinTypesInGraph(bool showBuiltinTypes) noexcept override;

  bool getShowDirectoryInCodeFileTitle() const noexcept override;
  void setShowDirectoryInCodeFileTitle(bool showDirectory) noexcept override;

  int getWindowBaseWidth() const noexcept override;
  int getWindowBaseHeight() const noexcept override;

  float getScrollSpeed() const noexcept override;
  void setScrollSpeed(float scrollSpeed) noexcept override;

  bool getGraphControlsVisible() const noexcept override;
  void setGraphControlsVisible(bool visible) noexcept override;

  GroupType getGraphGrouping() const noexcept override;
  void setGraphGrouping(GroupType type) noexcept override;

  // screen
  int getScreenAutoScaling() const noexcept override;
  void setScreenAutoScaling(int autoScaling) noexcept override;

  float getScreenScaleFactor() const noexcept override;
  void setScreenScaleFactor(float scaleFactor) noexcept override;

  // logging
  bool getLoggingEnabled() const noexcept override;
  void setLoggingEnabled(bool loggingEnabled) noexcept override;

  bool getVerboseIndexerLoggingEnabled() const noexcept override;
  void setVerboseIndexerLoggingEnabled(bool loggingEnabled) noexcept override;

  std::filesystem::path getLogDirectoryPath() const noexcept override;
  void setLogDirectoryPath(const std::filesystem::path& path) noexcept override;

  int getLogFilter() const noexcept override;
  void setLogFilter(int mask) noexcept override;

  int getStatusFilter() const noexcept override;
  void setStatusFilter(int mask) noexcept override;

  // indexing
  int getIndexerThreadCount() const noexcept override;
  void setIndexerThreadCount(int count) noexcept override;

  bool getMultiProcessIndexingEnabled() const noexcept override;
  void setMultiProcessIndexingEnabled(bool enabled) noexcept override;

  std::vector<std::filesystem::path> getHeaderSearchPaths() const noexcept override;
  std::vector<std::filesystem::path> getHeaderSearchPathsExpanded() const noexcept override;
  bool setHeaderSearchPaths(const std::vector<std::filesystem::path>& headerSearchPaths) noexcept override;

  bool getHasPrefilledHeaderSearchPaths() const noexcept override;
  void setHasPrefilledHeaderSearchPaths(bool v) noexcept override;

  std::vector<std::filesystem::path> getFrameworkSearchPaths() const noexcept override;
  std::vector<std::filesystem::path> getFrameworkSearchPathsExpanded() const noexcept override;
  bool setFrameworkSearchPaths(const std::vector<std::filesystem::path>& frameworkSearchPaths) noexcept override;

  bool getHasPrefilledFrameworkSearchPaths() const noexcept override;
  void setHasPrefilledFrameworkSearchPaths(bool v) noexcept override;

  // code
  int getCodeTabWidth() const noexcept override;
  void setCodeTabWidth(int codeTabWidth) noexcept override;

  int getCodeSnippetSnapRange() const noexcept override;
  void setCodeSnippetSnapRange(int range) noexcept override;

  int getCodeSnippetExpandRange() const noexcept override;
  void setCodeSnippetExpandRange(int range) noexcept override;

  bool getCodeViewModeSingle() const noexcept override;
  void setCodeViewModeSingle(bool enabled) noexcept override;

  // user
  std::vector<std::filesystem::path> getRecentProjects() const noexcept override;
  bool setRecentProjects(const std::vector<std::filesystem::path>& recentProjects) noexcept override;

  bool getSeenErrorHelpMessage() const noexcept override;
  void setSeenErrorHelpMessage(bool seen) noexcept override;

  std::filesystem::path getLastFilepickerLocation() const noexcept override;
  void setLastFilepickerLocation(const std::filesystem::path& path) noexcept override;

  float getGraphZoomLevel() const noexcept override;
  void setGraphZoomLevel(float zoomLevel) noexcept override;

  // network
  int getPluginPort() const noexcept override;
  void setPluginPort(int pluginPort) noexcept override;

  int getSourcetrailPort() const noexcept override;
  void setSourcetrailPort(int sourcetrailPort) noexcept override;

  // controls
  int getControlsMouseBackButton() const noexcept override;
  int getControlsMouseForwardButton() const noexcept override;

  bool getControlsGraphZoomOnMouseWheel() const noexcept override;
  void setControlsGraphZoomOnMouseWheel(bool zoomingDefault) noexcept override;
};
