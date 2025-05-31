/**
 * @file Project.h
 * @author Malte Langkabel (mlangkabel@coati.io)
 * @brief The Project class represents a project within the application.
 * @version 0.1
 * @date 2025-01-03
 *
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "IProject.hpp"
#if !defined(SOURCETRAIL_WASM)
#  include "RefreshInfo.h"
#  include "SourceGroup.h"
#  include "TaskGroupSequence.h"
#endif

class DialogView;
class FilePath;
class PersistentStorage;
class ProjectSettings;
class StorageCache;
struct FileInfo;
struct ProjectBuilderIndex;

/**
 * @brief The Project class represents a project within the application.
 *
 * This class is responsible for managing the project's settings, state, and operations such as loading, refreshing, and indexing.
 * It provides various methods to interact with the project, including checking its state, comparing settings, and managing
 * storage. The Project class is designed to be non-copyable and non-movable to ensure the integrity of the project data.
 *
 * Key functionalities include:
 * - Loading and refreshing the project
 * - Building the project index
 * - Managing project state and settings
 * - Interacting with dialog views for user interface operations
 *
 * The class also supports testing through the use of Google Test framework.
 */
class Project final : public IProject {
public:
  /**
   * @brief Constructor
   *
   * @param settings Project settings
   * @param storageCache Storage Cache
   * @param hasGUI The application started with GUI
   */
  Project(std::shared_ptr<ProjectSettings> settings, StorageCache* storageCache, std::string appUUID, bool hasGUI) noexcept;

  /**
   * @name Disable copy and move operators
   * @{ */
  Project(const Project&) = delete;
  Project(Project&&) = delete;
  Project& operator=(const Project&) = delete;
  Project& operator=(Project&&) = delete;
  /**  @} */

  ~Project() noexcept override;

  /**
   * @brief Get the project settings file path
   *
   * @return File path
   */
  [[nodiscard]] FilePath getProjectSettingsFilePath() const override;

  /**
   * @brief Get the project description
   *
   * @return Description
   */
  [[nodiscard]] std::string getDescription() const override;

  /**
   * @brief Check if the project is loaded
   *
   * @return True if loaded
   */
  [[nodiscard]] bool isLoaded() const override;

  /**
   * @brief Check if the project is indexing
   *
   * @return True if indexing
   */
  [[nodiscard]] bool isIndexing() const override;

  /**
   * @brief Check if the project settings are equal except name and location
   *
   * @param otherSettings Other project settings
   * @return True if equal
   */
  [[nodiscard]] bool settingsEqualExceptNameAndLocation(const ProjectSettings& otherSettings) const override;

  /**
   * @brief Set the project state to outdated
   */
  void setStateOutdated() override;

  /**
   * @brief Load the project
   *
   * @param dialogView Dialog view
   */
  void load(const std::shared_ptr<DialogView>& dialogView) override;

#if !defined(SOURCETRAIL_WASM)
  /**
   * @brief Refresh the project
   *
   * @param dialogView Dialog view
   * @param refreshMode Refresh mode
   * @param shallowIndexingRequested Shallow indexing requested
   */
  void refresh(std::shared_ptr<DialogView> dialogView, RefreshMode refreshMode, bool shallowIndexingRequested) override;

  /**
   * @brief Get the refresh info
   *
   * @param mode Refresh mode
   * @return Refresh info
   */
  [[nodiscard]] RefreshInfo getRefreshInfo(RefreshMode mode) const override;

  /**
   * @brief Build the index
   *
   * @param info Refresh info
   * @param dialogView Dialog view
   */
  void buildIndex(RefreshInfo info, std::shared_ptr<DialogView> dialogView) override;
#endif

private:
  enum class ProjectStateType : std::uint8_t { NOT_LOADED, EMPTY, LOADED, OUTDATED, OUTVERSIONED, NEEDS_MIGRATION, DB_CORRUPTED };

  enum class RefreshStageType : std::uint8_t { REFRESHING, INDEXING, NONE };

#if !defined(SOURCETRAIL_WASM)
  void swapToTempStorage(std::shared_ptr<DialogView> dialogView);
  bool swapToTempStorageFile(const FilePath& indexDbFilePath,
                             const FilePath& tempIndexDbFilePath,
                             std::shared_ptr<DialogView> dialogView);
  void discardTempStorage();

  [[nodiscard]] bool hasCxxSourceGroup() const;

  std::shared_ptr<TaskGroupSequence> createIndexTasks(RefreshInfo info,
                                                      std::shared_ptr<DialogView> dialogView,
                                                      std::shared_ptr<PersistentStorage> tempStorage,
                                                      size_t& sourceFileCount);

  bool checkIfNothingToRefresh(const RefreshInfo& info, std::shared_ptr<DialogView> dialogView);

  bool checkIfFilesToClear(RefreshInfo& info, std::shared_ptr<DialogView> dialogView);
#endif

  std::shared_ptr<ProjectSettings> m_settings;
  StorageCache* m_storageCache;

  ProjectStateType m_state = ProjectStateType::NOT_LOADED;
  RefreshStageType m_refreshStage = RefreshStageType::NONE;

  std::shared_ptr<PersistentStorage> m_storage;
#if !defined(SOURCETRAIL_WASM)
  std::vector<std::shared_ptr<SourceGroup>> m_sourceGroups;
#endif

  std::string m_appUUID;
  bool m_hasGUI;
};
