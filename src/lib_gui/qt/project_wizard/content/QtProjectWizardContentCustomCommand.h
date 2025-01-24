#pragma once

#include "QtProjectWizardContent.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox);
QT_FORWARD_DECLARE_CLASS(QLineEdit);

class SourceGroupSettingsCustomCommand;

class QtProjectWizardContentCustomCommand : public QtProjectWizardContent {
  // NOLINTNEXTLINE(readability-identifier-length)
  Q_OBJECT

public:
  QtProjectWizardContentCustomCommand(std::shared_ptr<SourceGroupSettingsCustomCommand> settings, QtProjectWizardWindow* window);

  // QtProjectWizardContent implementation
  void populate(QGridLayout* layout, int& row) override;

  void load() override;

  void save() override;

  bool check() override;

private:
  std::shared_ptr<SourceGroupSettingsCustomCommand> m_settings;

  QLineEdit* m_customCommand;
  QCheckBox* m_runInParallel;
};
