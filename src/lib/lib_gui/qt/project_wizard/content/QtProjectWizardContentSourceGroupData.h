#pragma once

#include "QtProjectWizardContent.h"
#include "SourceGroupStatusType.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox);
QT_FORWARD_DECLARE_CLASS(QLineEdit);

class SourceGroupSettings;

class QtProjectWizardContentSourceGroupData : public QtProjectWizardContent {
  Q_OBJECT

public:
  QtProjectWizardContentSourceGroupData(std::shared_ptr<SourceGroupSettings> settings, QtProjectWizardWindow* window);

  // QtProjectWizardContent implementation
  virtual void populate(QGridLayout* layout, int& row) override;

  virtual void load() override;
  virtual void save() override;
  virtual bool check() override;

signals:
  void nameUpdated(QString);
  void statusUpdated(SourceGroupStatusType);

private slots:
  void editedName(QString name);
  void changedStatus(bool checked);

private:
  std::shared_ptr<SourceGroupSettings> m_settings;

  QLineEdit* m_name;
  QCheckBox* m_status;
};
