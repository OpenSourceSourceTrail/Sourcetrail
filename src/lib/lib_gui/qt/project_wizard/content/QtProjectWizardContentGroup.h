#pragma once

#include "QtProjectWizardContent.h"

class QCheckBox;

class QtProjectWizardContentGroup : public QtProjectWizardContent {
  Q_OBJECT

public:
  explicit QtProjectWizardContentGroup(QtProjectWizardWindow* window);

  void addContent(QtProjectWizardContent* content);
  void addSpace();

  [[nodiscard]] bool hasContents() const;

protected:
  // QtProjectWizardContent implementation
  void populate(QGridLayout* layout, int& row) override;

  void load() override;
  void save() override;
  void refresh() override;
  bool check() override;

private:
  std::vector<QtProjectWizardContent*> m_contents;
};