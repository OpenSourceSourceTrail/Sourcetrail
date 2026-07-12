#pragma once
#include <memory>
#include <string>
#include <vector>

#include "LanguageType.h"
#include "Settings.h"
#include "SettingsMigrator.h"

class SourceGroupSettings;

class ProjectSettings : public Settings {
public:
  static const std::wstring PROJECT_FILE_EXTENSION;
  static const std::wstring BOOKMARK_DB_FILE_EXTENSION;
  static const std::wstring INDEX_DB_FILE_EXTENSION;
  static const std::wstring TEMP_INDEX_DB_FILE_EXTENSION;

  static const size_t VERSION;
  static LanguageType getLanguageOfProject(const FilePath& filePath);

  ProjectSettings();

  explicit ProjectSettings(const FilePath& projectFilePath);

  /**
   * @name Disable copy and move operators
   * @{ */
  ProjectSettings(const ProjectSettings&) = delete;
  ProjectSettings(ProjectSettings&&) = delete;
  ProjectSettings& operator=(const ProjectSettings&) = delete;
  ProjectSettings& operator=(ProjectSettings&&) = delete;
  /**  @} */

  ~ProjectSettings() override;

  [[nodiscard]] bool equalsExceptNameAndLocation(const ProjectSettings& other) const;

  [[nodiscard]] bool needMigration() const;

  void migrate();

  bool reload();

  [[nodiscard]] FilePath getProjectFilePath() const;

  void setProjectFilePath(std::wstring projectName, const FilePath& projectFileLocation);

  [[nodiscard]] FilePath getDependenciesDirectoryPath() const;

  [[nodiscard]] FilePath getDBFilePath() const;

  [[nodiscard]] FilePath getTempDBFilePath() const;

  [[nodiscard]] FilePath getBookmarkDBFilePath() const;

  [[nodiscard]] std::wstring getProjectName() const;

  [[nodiscard]] FilePath getProjectDirectoryPath() const;

  [[nodiscard]] std::string getDescription() const;

  [[nodiscard]] std::vector<std::shared_ptr<SourceGroupSettings>> getAllSourceGroupSettings() const;

  void setAllSourceGroupSettings(const std::vector<std::shared_ptr<SourceGroupSettings>>& allSettings);

  [[nodiscard]] std::vector<FilePath> makePathsExpandedAndAbsolute(const std::vector<FilePath>& paths) const;

private:
  SettingsMigrator getMigrations() const;
};
