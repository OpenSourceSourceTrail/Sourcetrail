#pragma once
#include <cassert>

#include <Settings.h>

#include "GroupType.h"

struct IApplicationSettings : Settings {
  using Ptr = std::shared_ptr<IApplicationSettings>;

  static Ptr getInstance() noexcept {
    assert(mInstance);
    return mInstance;
  }

  static IApplicationSettings* getInstanceRaw() noexcept {
    assert(mInstance);
    return mInstance.get();
  }

  static void setInstance(Ptr instance) noexcept {
    mInstance = std::move(instance);
  }

  IApplicationSettings() noexcept;
  ~IApplicationSettings() noexcept override;

  [[nodiscard]] bool operator==(const IApplicationSettings& other) const noexcept;

  [[nodiscard]] virtual size_t getMaxRecentProjectsCount() const noexcept = 0;

  // application
  [[nodiscard]] virtual std::string getFontName() const noexcept = 0;
  virtual void setFontName(const std::string& fontName) noexcept = 0;

  [[nodiscard]] virtual int getFontSize() const noexcept = 0;
  virtual void setFontSize(int fontSize) noexcept = 0;

  [[nodiscard]] virtual std::string getTextEncoding() const noexcept = 0;
  virtual void setTextEncoding(const std::string& textEncoding) noexcept = 0;

  [[nodiscard]] virtual std::wstring getColorSchemeName() const noexcept = 0;
  [[nodiscard]] virtual std::filesystem::path getColorSchemePath() const noexcept = 0;
  virtual void setColorSchemeName(const std::wstring& colorSchemeName) noexcept = 0;

  [[nodiscard]] virtual int getFontSizeMax() const noexcept = 0;
  virtual void setFontSizeMax(int fontSizeMax) noexcept = 0;

  [[nodiscard]] virtual int getFontSizeMin() const noexcept = 0;
  virtual void setFontSizeMin(int fontSizeMin) noexcept = 0;

  [[nodiscard]] virtual int getFontSizeStd() const noexcept = 0;
  virtual void setFontSizeStd(int fontSizeStd) noexcept = 0;

  [[nodiscard]] virtual bool getUseAnimations() const noexcept = 0;
  virtual void setUseAnimations(bool useAnimations) noexcept = 0;

  [[nodiscard]] virtual bool getShowBuiltinTypesInGraph() const noexcept = 0;
  virtual void setShowBuiltinTypesInGraph(bool showBuiltinTypes) noexcept = 0;

  [[nodiscard]] virtual bool getShowDirectoryInCodeFileTitle() const noexcept = 0;
  virtual void setShowDirectoryInCodeFileTitle(bool showDirectory) = 0;

  [[nodiscard]] virtual int getWindowBaseWidth() const noexcept = 0;
  virtual int getWindowBaseHeight() const noexcept = 0;

  [[nodiscard]] virtual float getScrollSpeed() const noexcept = 0;
  virtual void setScrollSpeed(float scrollSpeed) noexcept = 0;

  [[nodiscard]] virtual bool getGraphControlsVisible() const noexcept = 0;
  virtual void setGraphControlsVisible(bool visible) noexcept = 0;

  [[nodiscard]] virtual GroupType getGraphGrouping() const noexcept = 0;
  virtual void setGraphGrouping(GroupType type) noexcept = 0;

  // screen
  [[nodiscard]] virtual int getScreenAutoScaling() const noexcept = 0;
  virtual void setScreenAutoScaling(int autoScaling) noexcept = 0;

  [[nodiscard]] virtual float getScreenScaleFactor() const noexcept = 0;
  virtual void setScreenScaleFactor(float scaleFactor) noexcept = 0;

  /**
   * @name Logging
   */
  /** @{ */
  [[nodiscard]] virtual bool getLoggingEnabled() const noexcept = 0;
  virtual void setLoggingEnabled(bool loggingEnabled) noexcept = 0;

  [[nodiscard]] virtual bool getVerboseIndexerLoggingEnabled() const noexcept = 0;
  virtual void setVerboseIndexerLoggingEnabled(bool loggingEnabled) noexcept = 0;

  [[nodiscard]] virtual std::filesystem::path getLogDirectoryPath() const noexcept = 0;
  virtual void setLogDirectoryPath(const std::filesystem::path& path) noexcept = 0;

  [[nodiscard]] virtual int getLogFilter() const noexcept = 0;
  virtual void setLogFilter(int mask) noexcept = 0;

  [[nodiscard]] virtual int getStatusFilter() const noexcept = 0;
  virtual void setStatusFilter(int mask) noexcept = 0;

  /**
   * @brief Get the Logging Level object
   *
   * @note level must map to spdlog::level::level_enum
   *
   * @return int Logging level
   */
  [[nodiscard]] virtual int getLoggingLevel() const noexcept = 0;

  /**
   * @brief Set the Logging Level object
   *
   * @note level must map to spdlog::level::level_enum
   *
   * @param level Logging level
   */
  virtual void setLoggingLevel(int level) noexcept = 0;
  /** @} */

  // indexing
  [[nodiscard]] virtual int getIndexerThreadCount() const noexcept = 0;
  virtual void setIndexerThreadCount(int count) noexcept = 0;

  [[nodiscard]] virtual bool getMultiProcessIndexingEnabled() const noexcept = 0;
  virtual void setMultiProcessIndexingEnabled(bool enabled) noexcept = 0;

  [[nodiscard]] virtual std::vector<std::filesystem::path> getHeaderSearchPaths() const noexcept = 0;
  [[nodiscard]] virtual std::vector<std::filesystem::path> getHeaderSearchPathsExpanded() const noexcept = 0;
  virtual bool setHeaderSearchPaths(const std::vector<std::filesystem::path>& headerSearchPaths) noexcept = 0;

  [[nodiscard]] virtual bool getHasPrefilledHeaderSearchPaths() const noexcept = 0;
  virtual void setHasPrefilledHeaderSearchPaths(bool v) noexcept = 0;

  [[nodiscard]] virtual std::vector<std::filesystem::path> getFrameworkSearchPaths() const noexcept = 0;
  [[nodiscard]] virtual std::vector<std::filesystem::path> getFrameworkSearchPathsExpanded() const noexcept = 0;
  virtual bool setFrameworkSearchPaths(const std::vector<std::filesystem::path>& frameworkSearchPaths) noexcept = 0;

  [[nodiscard]] virtual bool getHasPrefilledFrameworkSearchPaths() const noexcept = 0;
  virtual void setHasPrefilledFrameworkSearchPaths(bool v) noexcept = 0;

  // code
  [[nodiscard]] virtual int getCodeTabWidth() const noexcept = 0;
  virtual void setCodeTabWidth(int codeTabWidth) noexcept = 0;

  [[nodiscard]] virtual int getCodeSnippetSnapRange() const noexcept = 0;
  virtual void setCodeSnippetSnapRange(int range) noexcept = 0;

  [[nodiscard]] virtual int getCodeSnippetExpandRange() const noexcept = 0;
  virtual void setCodeSnippetExpandRange(int range) noexcept = 0;

  [[nodiscard]] virtual bool getCodeViewModeSingle() const noexcept = 0;
  virtual void setCodeViewModeSingle(bool enabled) noexcept = 0;

  // user
  [[nodiscard]] virtual std::vector<std::filesystem::path> getRecentProjects() const noexcept = 0;
  virtual bool setRecentProjects(const std::vector<std::filesystem::path>& recentProjects) noexcept = 0;

  [[nodiscard]] virtual bool getSeenErrorHelpMessage() const noexcept = 0;
  virtual void setSeenErrorHelpMessage(bool seen) noexcept = 0;

  [[nodiscard]] virtual std::filesystem::path getLastFilepickerLocation() const noexcept = 0;
  virtual void setLastFilepickerLocation(const std::filesystem::path& path) noexcept = 0;

  [[nodiscard]] virtual float getGraphZoomLevel() const noexcept = 0;
  virtual void setGraphZoomLevel(float zoomLevel) noexcept = 0;

  // network
  [[nodiscard]] virtual int getPluginPort() const noexcept = 0;
  virtual void setPluginPort(int pluginPort) noexcept = 0;

  [[nodiscard]] virtual int getSourcetrailPort() const noexcept = 0;
  virtual void setSourcetrailPort(int sourcetrailPort) noexcept = 0;

  // controls
  [[nodiscard]] virtual int getControlsMouseBackButton() const noexcept = 0;
  [[nodiscard]] virtual int getControlsMouseForwardButton() const noexcept = 0;

  [[nodiscard]] virtual bool getControlsGraphZoomOnMouseWheel() const noexcept = 0;
  virtual void setControlsGraphZoomOnMouseWheel(bool zoomingDefault) noexcept = 0;

private:
  static Ptr mInstance;
};
