#pragma once

#include "QtProjectWizardContent.h"

class QtProjectWizardContentSourceGroupInfoText : public QtProjectWizardContent {
  Q_OBJECT

public:
  QtProjectWizardContentSourceGroupInfoText(QtProjectWizardWindow* window);

  // QtProjectWizardContent implementation
  void populate(QGridLayout* layout, int& row) override;
};
