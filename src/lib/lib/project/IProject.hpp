/**
 * @file IProject.hpp
 * @author Ahmed Abdel Aal (eng.ahmedhussein89@gmail.com)
 * @brief The Project class represents a project within the application.
 * @version 0.1
 * @date 2025-01-03
 *
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <DialogView.h>
#include <FilePath.h>
#include <ProjectSettings.h>

/**
 * @brief The IProject class represents an interface for a project within the application.
 */
struct IProject {
  /**
   * @brief Destructor
   */
  virtual ~IProject() noexcept;

  /**
   * @brief Get the project settings file path
   *
   * @return File path
   */
  [[nodiscard]] virtual FilePath getProjectSettingsFilePath() const = 0;

  /**
   * @brief Get the project description
   *
   * @return Description
   */
  [[nodiscard]] virtual std::string getDescription() const = 0;

  /**
   * @brief Check if the project is loaded
   *
   * @return True if loaded
   */
  [[nodiscard]] virtual bool isLoaded() const = 0;

  /**
   * @brief Check if the project is indexing
   *
   * @return True if indexing
   */
  [[nodiscard]] virtual bool isIndexing() const = 0;

  /**
   * @brief Check if the project settings are equal except name and location
   *
   * @param otherSettings Other project settings
   * @return True if equal
   */
  [[nodiscard]] virtual bool settingsEqualExceptNameAndLocation(const ProjectSettings& otherSettings) const = 0;

  /**
   * @brief Load the project
   *
   * @param dialogView Dialog view
   */
  virtual void setStateOutdated() = 0;

  /**
   * @brief Load the project
   *
   * @param dialogView Dialog view
   */
  virtual void load(const std::shared_ptr<DialogView>& dialogView) = 0;

  /**
   * @brief Refresh the project
   *
   * @param dialogView Dialog view
   * @param refreshMode Refresh mode
   * @param shallowIndexingRequested True if shallow indexing requested
   */
  virtual void refresh(std::shared_ptr<DialogView> dialogView, RefreshMode refreshMode, bool shallowIndexingRequested) = 0;

  /**
   * @brief Get the refresh info
   *
   * @param mode Refresh mode
   * @return Refresh info
   */
  [[nodiscard]] virtual RefreshInfo getRefreshInfo(RefreshMode mode) const = 0;

  /**
   * @brief Build the index
   *
   * @param info Refresh info
   * @param dialogView Dialog view
   */
  virtual void buildIndex(RefreshInfo info, std::shared_ptr<DialogView> dialogView) = 0;
};
