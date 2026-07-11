#pragma once

#include "QtProjectWizardContent.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox);

class QtStringListBox;
class SourceGroupSettingsWithCxxPchOptions;

class QtProjectWizardContentCxxPchFlags : public QtProjectWizardContent {
  Q_OBJECT

public:
  QtProjectWizardContentCxxPchFlags(std::shared_ptr<SourceGroupSettingsWithCxxPchOptions> settings,
                                    QtProjectWizardWindow* window,
                                    bool isCDB);

  // QtProjectWizardContent implementation
  virtual void populate(QGridLayout* layout, int& row) override;

  virtual void load() override;
  virtual void save() override;
  virtual bool check() override;

private:
  std::shared_ptr<SourceGroupSettingsWithCxxPchOptions> m_settings;
  const bool m_isCDB;

  QCheckBox* m_useCompilerFlags;
  QtStringListBox* m_list;
};
