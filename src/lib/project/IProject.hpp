#pragma once
#include <DialogView.h>
#include <FilePath.h>
#include <ProjectSettings.h>

struct IProject {
  virtual ~IProject();
  [[nodiscard]] virtual FilePath getProjectSettingsFilePath() const = 0;
  [[nodiscard]] virtual std::string getDescription() const = 0;

  [[nodiscard]] virtual bool isLoaded() const = 0;
  [[nodiscard]] virtual bool isIndexing() const = 0;

  [[nodiscard]] virtual bool settingsEqualExceptNameAndLocation(const ProjectSettings& otherSettings) const = 0;
  virtual void setStateOutdated() = 0;

  virtual void load(const std::shared_ptr<DialogView>& dialogView) = 0;

  virtual void refresh(std::shared_ptr<DialogView> dialogView, RefreshMode refreshMode, bool shallowIndexingRequested) = 0;

  [[nodiscard]] virtual RefreshInfo getRefreshInfo(RefreshMode mode) const = 0;

  virtual void buildIndex(RefreshInfo info, std::shared_ptr<DialogView> dialogView) = 0;
};