#include "details/ApplicationSettings.h"

#include <filesystem>
#include <memory>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include <system_error>

#include "ResourcePaths.h"
#include "SettingsMigrationLambda.h"
#include "SettingsMigrationMoveKey.h"
#include "SettingsMigrator.h"
#include "Status.h"
#include "UserPaths.h"
#include "utility.h"
#include "utilityFile.h"

namespace fs = std::filesystem;

const size_t ApplicationSettings::VERSION = 8;

ApplicationSettings::ApplicationSettings() = default;
ApplicationSettings::~ApplicationSettings() = default;

bool ApplicationSettings::load(const FilePath& filePath, bool readOnly) {
  if(!Settings::load(filePath, readOnly)) {
    return false;
  }

  SettingsMigrator migrator;

  migrator.addMigration(1,
                        std::make_shared<SettingsMigrationMoveKey>("source/header_search_paths/header_search_path",
                                                                   "indexing/cxx/header_search_paths/header_search_path"));
  migrator.addMigration(1,
                        std::make_shared<SettingsMigrationMoveKey>("source/framework_search_paths/framework_search_path",
                                                                   "indexing/cxx/framework_search_paths/framework_search_path"));
  migrator.addMigration(
      1, std::make_shared<SettingsMigrationMoveKey>("application/indexer_thread_count", "indexing/indexer_thread_count"));
  migrator.addMigration(2, std::make_shared<SettingsMigrationMoveKey>("network/coati_port", "network/sourcetrail_port"));
  migrator.addMigration(4, std::make_shared<SettingsMigrationLambda>([](const SettingsMigration* migration, Settings* settings) {
                          std::wstring colorSchemePathString = migration->getValueFromSettings<std::wstring>(
                              settings, "application/color_scheme", L"");
                          if(!colorSchemePathString.empty()) {
                            FilePath colorSchemePath(colorSchemePathString);
                            migration->setValueInSettings(
                                settings, "application/color_scheme", colorSchemePath.withoutExtension().fileName());
                          }
                        }));
  migrator.addMigration(7, std::make_shared<SettingsMigrationLambda>([](const SettingsMigration* migration, Settings* settings) {
                          std::vector<std::string> recentProjects;
                          recentProjects.push_back("./projects/tictactoe_py/tictactoe_py.srctrlprj");
                          utility::append(recentProjects,
                                          migration->getValuesFromSettings(
                                              settings, "user/recent_projects/recent_project", std::vector<std::string>()));

                          for(size_t i = 0; i < recentProjects.size(); i++) {
                            if(recentProjects[i] == "./projects/tictactoe/tictactoe.srctrlprj") {
                              recentProjects[i] = "./projects/tictactoe_cpp/tictactoe_cpp.srctrlprj";
                            }
                          }
                          migration->setValuesInSettings(settings, "user/recent_projects/recent_project", recentProjects);
                        }));
  migrator.addMigration(8, std::make_shared<SettingsMigrationLambda>([](const SettingsMigration* migration, Settings* settings) {
                          std::vector<FilePath> cxxHeaderSearchPaths = migration->getValuesFromSettings(
                              settings, "indexing/cxx/header_search_paths/header_search_path", std::vector<FilePath>());

                          std::vector<FilePath> newCxxHeaderSearchPaths;
                          for(const FilePath& path : cxxHeaderSearchPaths) {
                            if(path.getCanonical().getConcatenated(L"/stdarg.h").exists() &&
                               path.str().find("data/cxx/include") != std::string::npos) {
                              continue;
                            }

                            newCxxHeaderSearchPaths.push_back(path);
                          }

                          migration->setValuesInSettings(
                              settings, "indexing/cxx/header_search_paths/header_search_path", newCxxHeaderSearchPaths);

                          if(newCxxHeaderSearchPaths.size() == 0) {
                            migration->setValueInSettings(settings, "indexing/cxx/has_prefilled_header_search_paths", false);
                          }
                        }));
  bool migrated = migrator.migrate(this, ApplicationSettings::VERSION);
  if(migrated) {
    save();
  }

  return true;
}

size_t ApplicationSettings::getMaxRecentProjectsCount() const noexcept {
  return 7;
}

std::string ApplicationSettings::getFontName() const noexcept {
  return getValue<std::string>("application/font_name", "Source Code Pro");
}

void ApplicationSettings::setFontName(const std::string& fontName) noexcept {
  setValue<std::string>("application/font_name", fontName);
}

int ApplicationSettings::getFontSize() const noexcept {
  return getValue<int>("application/font_size", 14);
}

void ApplicationSettings::setFontSize(int fontSize) noexcept {
  setValue<int>("application/font_size", fontSize);
}

std::string ApplicationSettings::getTextEncoding() const noexcept {
  return getValue<std::string>("application/text_encoding", "UTF-8");
}

void ApplicationSettings::setTextEncoding(const std::string& textEncoding) noexcept {
  setValue<std::string>("application/text_encoding", textEncoding);
}

bool ApplicationSettings::getUseAnimations() const noexcept {
  return getValue<bool>("application/use_animations", true);
}

void ApplicationSettings::setUseAnimations(bool useAnimations) noexcept {
  setValue<bool>("application/use_animations", useAnimations);
}

bool ApplicationSettings::getShowBuiltinTypesInGraph() const noexcept {
  return getValue<bool>("application/builtin_types_in_graph", false);
}

void ApplicationSettings::setShowBuiltinTypesInGraph(bool showBuiltinTypes) noexcept {
  setValue<bool>("application/builtin_types_in_graph", showBuiltinTypes);
}

bool ApplicationSettings::getShowDirectoryInCodeFileTitle() const noexcept {
  return getValue<bool>("application/directory_in_code_title", false);
}

void ApplicationSettings::setShowDirectoryInCodeFileTitle(bool showDirectory) noexcept {
  setValue<bool>("application/directory_in_code_title", showDirectory);
}

std::wstring ApplicationSettings::getColorSchemeName() const noexcept {
  return getValue<std::wstring>("application/color_scheme", L"bright");
}

fs::path ApplicationSettings::getColorSchemePath() const noexcept {
  fs::path defaultPath(ResourcePaths::getColorSchemesDirectoryPath().concatenate(L"bright.xml").wstr());
  fs::path path(ResourcePaths::getColorSchemesDirectoryPath().concatenate(getColorSchemeName() + L".xml").wstr());

  std::error_code errorCode;
  if(path != defaultPath && !fs::exists(path, errorCode)) {
    return defaultPath;
  }

  return path;
}

void ApplicationSettings::setColorSchemeName(const std::wstring& colorSchemeName) noexcept {
  setValue("application/color_scheme", colorSchemeName);
}

int ApplicationSettings::getFontSizeMax() const noexcept {
  return getValue<int>("application/font_size_max", 24);
}

void ApplicationSettings::setFontSizeMax(const int fontSizeMax) noexcept {
  setValue<int>("application/font_size_max", fontSizeMax);
}

int ApplicationSettings::getFontSizeMin() const noexcept {
  return getValue<int>("application/font_size_min", 4);
}

void ApplicationSettings::setFontSizeMin(const int fontSizeMin) noexcept {
  setValue<int>("application/font_size_min", fontSizeMin);
}

int ApplicationSettings::getFontSizeStd() const noexcept {
  return getValue<int>("application/font_size_std", 12);
}

void ApplicationSettings::setFontSizeStd(int fontSizeStd) noexcept {
  setValue<int>("application/font_size_std", fontSizeStd);
}

int ApplicationSettings::getWindowBaseWidth() const noexcept {
  return getValue<int>("application/window_base_width", 500);
}

int ApplicationSettings::getWindowBaseHeight() const noexcept {
  return getValue<int>("application/window_base_height", 500);
}

float ApplicationSettings::getScrollSpeed() const noexcept {
  return getValue<float>("application/scroll_speed", 1.0f);
}

void ApplicationSettings::setScrollSpeed(float scrollSpeed) noexcept {
  setValue<float>("application/scroll_speed", scrollSpeed);
}

bool ApplicationSettings::getGraphControlsVisible() const noexcept {
  return getValue<bool>("application/graph_controls_visible", true);
}

void ApplicationSettings::setGraphControlsVisible(bool visible) noexcept {
  setValue<bool>("application/graph_controls_visible", visible);
}

GroupType ApplicationSettings::getGraphGrouping() const noexcept {
  return stringToGroupType(getValue<std::wstring>("application/graph_grouping", groupTypeToString(GroupType::NONE)));
}

void ApplicationSettings::setGraphGrouping(GroupType type) noexcept {
  setValue<std::wstring>("application/graph_grouping", groupTypeToString(type));
}

int ApplicationSettings::getScreenAutoScaling() const noexcept {
  return getValue<int>("screen/auto_scaling", 1);
}

void ApplicationSettings::setScreenAutoScaling(int autoScaling) noexcept {
  setValue<int>("screen/auto_scaling", autoScaling);
}

float ApplicationSettings::getScreenScaleFactor() const noexcept {
  return getValue<float>("screen/scale_factor", -1.0);
}

void ApplicationSettings::setScreenScaleFactor(float scaleFactor) noexcept {
  setValue<float>("screen/scale_factor", scaleFactor);
}

bool ApplicationSettings::getLoggingEnabled() const noexcept {
  return getValue<bool>("application/logging_enabled", true);
}

void ApplicationSettings::setLoggingEnabled(bool value) noexcept {
  setValue<bool>("application/logging_enabled", value);
}

bool ApplicationSettings::getVerboseIndexerLoggingEnabled() const noexcept {
  return getValue<bool>("application/verbose_indexer_logging_enabled", false);
}

void ApplicationSettings::setVerboseIndexerLoggingEnabled(bool value) noexcept {
  setValue<bool>("application/verbose_indexer_logging_enabled", value);
}

fs::path ApplicationSettings::getLogDirectoryPath() const noexcept {
  return getValue<std::wstring>("application/log_directory_path", UserPaths::getLogDirectoryPath().getAbsolute().wstr());
}

void ApplicationSettings::setLogDirectoryPath(const fs::path& path) noexcept {
  setValue<std::wstring>("application/log_directory_path", path.wstring());
}

void ApplicationSettings::setLogFilter(int mask) noexcept {
  setValue<int>("application/log_filter", mask);
}

void ApplicationSettings::setStatusFilter(int mask) noexcept {
  setValue<int>("application/status_filter", mask);
}

int ApplicationSettings::getStatusFilter() const noexcept {
  return getValue<int>("application/status_filter", StatusType::STATUS_INFO | StatusType::STATUS_ERROR);
}

int ApplicationSettings::getLogFilter() const noexcept {
  // FIXME
  return false;
  // return getValue<int>("application/log_filter", Logger::LOG_WARNINGS | Logger::LOG_ERRORS);
}

int ApplicationSettings::getIndexerThreadCount() const noexcept {
  return getValue<int>("indexing/indexer_thread_count", 0);
}

void ApplicationSettings::setIndexerThreadCount(const int count) noexcept {
  setValue<int>("indexing/indexer_thread_count", count);
}

bool ApplicationSettings::getMultiProcessIndexingEnabled() const noexcept {
  return getValue<bool>("indexing/multi_process_indexing", true);
}

void ApplicationSettings::setMultiProcessIndexingEnabled(bool enabled) noexcept {
  setValue<bool>("indexing/multi_process_indexing", enabled);
}

std::vector<fs::path> ApplicationSettings::getHeaderSearchPaths() const noexcept {
  return getPathValuesStl("indexing/cxx/header_search_paths/header_search_path");
}

std::vector<fs::path> ApplicationSettings::getHeaderSearchPathsExpanded() const noexcept {
  return utility::getExpandedPaths(getHeaderSearchPaths());
}

bool ApplicationSettings::setHeaderSearchPaths(const std::vector<fs::path>& headerSearchPaths) noexcept {
  return setPathValues("indexing/cxx/header_search_paths/header_search_path", headerSearchPaths);
}

bool ApplicationSettings::getHasPrefilledHeaderSearchPaths() const noexcept {
  return getValue<bool>("indexing/cxx/has_prefilled_header_search_paths", false);
}

void ApplicationSettings::setHasPrefilledHeaderSearchPaths(bool v) noexcept {
  setValue<bool>("indexing/cxx/has_prefilled_header_search_paths", v);
}

std::vector<fs::path> ApplicationSettings::getFrameworkSearchPaths() const noexcept {
  return getPathValuesStl("indexing/cxx/framework_search_paths/framework_search_path");
}

std::vector<fs::path> ApplicationSettings::getFrameworkSearchPathsExpanded() const noexcept {
  return utility::getExpandedPaths(getFrameworkSearchPaths());
}

bool ApplicationSettings::setFrameworkSearchPaths(const std::vector<fs::path>& frameworkSearchPaths) noexcept {
  return setPathValues("indexing/cxx/framework_search_paths/framework_search_path", frameworkSearchPaths);
}

bool ApplicationSettings::getHasPrefilledFrameworkSearchPaths() const noexcept {
  return getValue<bool>("indexing/cxx/has_prefilled_framework_search_paths", false);
}

void ApplicationSettings::setHasPrefilledFrameworkSearchPaths(bool v) noexcept {
  setValue<bool>("indexing/cxx/has_prefilled_framework_search_paths", v);
}

int ApplicationSettings::getCodeTabWidth() const noexcept {
  return getValue<int>("code/tab_width", 4);
}

void ApplicationSettings::setCodeTabWidth(int codeTabWidth) noexcept {
  setValue<int>("code/tab_width", codeTabWidth);
}

int ApplicationSettings::getCodeSnippetSnapRange() const noexcept {
  return getValue<int>("code/snippet/snap_range", 4);
}

void ApplicationSettings::setCodeSnippetSnapRange(int range) noexcept {
  setValue<int>("code/snippet/snap_range", range);
}

int ApplicationSettings::getCodeSnippetExpandRange() const noexcept {
  return getValue<int>("code/snippet/expand_range", 3);
}

void ApplicationSettings::setCodeSnippetExpandRange(int range) noexcept {
  setValue<int>("code/snippet/expand_range", range);
}

bool ApplicationSettings::getCodeViewModeSingle() const noexcept {
  return getValue<bool>("code/view_mode_single", false);
}

void ApplicationSettings::setCodeViewModeSingle(bool enabled) noexcept {
  setValue<bool>("code/view_mode_single", enabled);
}

std::vector<std::filesystem::path> ApplicationSettings::getRecentProjects() const noexcept {
  constexpr auto RecentProjectKey = "user/recent_projects/recent_project";
  return getPathValuesStl(RecentProjectKey);
}

bool ApplicationSettings::setRecentProjects(const std::vector<fs::path>& recentProjects) noexcept {
  return setPathValues("user/recent_projects/recent_project", recentProjects);
}

bool ApplicationSettings::getSeenErrorHelpMessage() const noexcept {
  return getValue<bool>("user/seen_error_help_message", false);
}

void ApplicationSettings::setSeenErrorHelpMessage(bool seen) noexcept {
  setValue<bool>("user/seen_error_help_message", seen);
}

fs::path ApplicationSettings::getLastFilepickerLocation() const noexcept {
  return getValue<std::wstring>("user/last_filepicker_location", L"");
}

void ApplicationSettings::setLastFilepickerLocation(const fs::path& path) noexcept {
  setValue<std::wstring>("user/last_filepicker_location", path.wstring());
}

float ApplicationSettings::getGraphZoomLevel() const noexcept {
  return getValue<float>("user/graph_zoom_level", 1.0f);
}

void ApplicationSettings::setGraphZoomLevel(float zoomLevel) noexcept {
  setValue<float>("user/graph_zoom_level", zoomLevel);
}

int ApplicationSettings::getPluginPort() const noexcept {
  return getValue<int>("network/plugin_port", 6666);
}

void ApplicationSettings::setPluginPort(const int pluginPort) noexcept {
  setValue<int>("network/plugin_port", pluginPort);
}

int ApplicationSettings::getSourcetrailPort() const noexcept {
  return getValue<int>("network/sourcetrail_port", 6667);
}

void ApplicationSettings::setSourcetrailPort(const int sourcetrailPort) noexcept {
  setValue<int>("network/sourcetrail_port", sourcetrailPort);
}

int ApplicationSettings::getControlsMouseBackButton() const noexcept {
  return getValue<int>("controls/mouse_back_button", 0x8);
}

int ApplicationSettings::getControlsMouseForwardButton() const noexcept {
  return getValue<int>("controls/mouse_forward_button", 0x10);
}

bool ApplicationSettings::getControlsGraphZoomOnMouseWheel() const noexcept {
  return getValue<bool>("controls/graph_zoom_on_mouse_wheel", false);
}

void ApplicationSettings::setControlsGraphZoomOnMouseWheel(bool zoomingDefault) noexcept {
  setValue<bool>("controls/graph_zoom_on_mouse_wheel", zoomingDefault);
}
