#pragma once
// STL
#include <memory>

#include "QtProjectWizardContent.h"

class QtStringListBox;
class SourceGroupSettingsWithSourceExtensions;

class QtProjectWizardContentExtensions : public QtProjectWizardContent {
  Q_OBJECT

public:
  QtProjectWizardContentExtensions(std::shared_ptr<SourceGroupSettingsWithSourceExtensions> settings,
                                   QtProjectWizardWindow* window);

  // QtProjectWizardContent implementation
  virtual void populate(QGridLayout* layout, int& row) override;

  virtual void load() override;
  virtual void save() override;

private:
  std::shared_ptr<SourceGroupSettingsWithSourceExtensions> m_settings;

  QtStringListBox* m_listBox;
};
