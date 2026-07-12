#pragma once
#include <memory>
#include <vector>

#include <QString>

#include "FilePath.h"
#include "SingleValueCache.h"

class QtProjectWizardWindow;
class SourceGroupSettingsCxxCdb;

class ProjectWizardModel final {
public:
  explicit ProjectWizardModel(std::shared_ptr<SourceGroupSettingsCxxCdb> settings) noexcept;

  ProjectWizardModel(const ProjectWizardModel&) = delete;
  ProjectWizardModel(ProjectWizardModel&&) = delete;
  ProjectWizardModel& operator=(const ProjectWizardModel&) = delete;
  ProjectWizardModel& operator=(ProjectWizardModel&&) = delete;

  ~ProjectWizardModel() noexcept;

  void onPickerTextChanged(QtProjectWizardWindow* window, const QString& text);

  void pickedPath(QtProjectWizardWindow* window);

  const std::shared_ptr<SourceGroupSettingsCxxCdb>& settings() const {
    return m_settings;
  }

  void clearFilePaths() {
    m_filePaths.clear();
  }

  std::vector<FilePath> filePaths() const {
    return m_filePaths.getValue();
  }

private:
  std::shared_ptr<SourceGroupSettingsCxxCdb> m_settings;
  mutable SingleValueCache<std::vector<FilePath>> m_filePaths;
};