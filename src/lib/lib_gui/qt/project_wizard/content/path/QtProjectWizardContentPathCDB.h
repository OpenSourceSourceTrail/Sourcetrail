#pragma once

#include "QtProjectWizardContentPath.h"

class ProjectWizardModel;

class QtProjectWizardContentPathCDB final : public QtProjectWizardContentPath {
  // NOLINTNEXTLINE(readability-identifier-length)
  Q_OBJECT

public:
  QtProjectWizardContentPathCDB(std::shared_ptr<ProjectWizardModel> model, QtProjectWizardWindow* window);

  ~QtProjectWizardContentPathCDB() override;

  // QtProjectWizardContent implementation
  void populate(QGridLayout* layout, int& row) override;

  void load() override;
  void save() override;
  void refresh() override;

  [[nodiscard]] std::vector<FilePath> getFilePaths() const override;
  [[nodiscard]] QString getFileNamesTitle() const override;
  [[nodiscard]] QString getFileNamesDescription() const override;

private:
  std::shared_ptr<SourceGroupSettings> getSourceGroupSettings() override;

  std::shared_ptr<ProjectWizardModel> m_model;
  QLabel* m_fileCountLabel = nullptr;
};
