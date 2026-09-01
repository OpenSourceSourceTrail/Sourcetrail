#pragma once
#include <memory>
#include <vector>

#include <QFutureWatcher>
#include <QString>

#include "FilePath.h"

class QtProjectWizardWindow;
class SourceGroupSettingsCxxCdb;

class ProjectWizardModel final {
public:
  /** What reading the compilation database yields; computed off the GUI thread. */
  struct CdbInfo {
    bool valid = false;
    std::vector<FilePath> indexedHeaderPaths;
    std::vector<FilePath> filePaths;
  };

  explicit ProjectWizardModel(std::shared_ptr<SourceGroupSettingsCxxCdb> settings) noexcept;

  ProjectWizardModel(const ProjectWizardModel&) = delete;
  ProjectWizardModel(ProjectWizardModel&&) = delete;
  ProjectWizardModel& operator=(const ProjectWizardModel&) = delete;
  ProjectWizardModel& operator=(ProjectWizardModel&&) = delete;

  ~ProjectWizardModel() noexcept;

  void onPickerTextChanged(QtProjectWizardWindow* window, const QString& text);

  void pickedPath(QtProjectWizardWindow* window);

  /** Starts reading the database in the background unless its result is already there. */
  void ensureUpToDate(QtProjectWizardWindow* window);

  [[nodiscard]] bool isLoading() const {
    return m_watcher.isRunning();
  }

  const std::shared_ptr<SourceGroupSettingsCxxCdb>& settings() const {
    return m_settings;
  }

  std::vector<FilePath> filePaths() const {
    return m_filePaths;
  }

private:
  std::shared_ptr<SourceGroupSettingsCxxCdb> m_settings;
  /** The path the running or finished read belongs to, so the same one is not read twice. */
  FilePath m_requestedPath;
  std::vector<FilePath> m_filePaths;
  QFutureWatcher<CdbInfo> m_watcher;
};
