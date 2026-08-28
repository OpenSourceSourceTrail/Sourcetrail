#pragma once

#include "qt/project_wizard/content/QtProjectWizardContent.h"

class QtProjectWizardContentSourceGroupInfoText : public QtProjectWizardContent {
  Q_OBJECT

public:
  QtProjectWizardContentSourceGroupInfoText(QtProjectWizardWindow* window);

  // QtProjectWizardContent implementation
  void populate(QGridLayout* layout, int& row) override;
};
