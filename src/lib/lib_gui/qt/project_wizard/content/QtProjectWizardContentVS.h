#pragma once

#include "QtProjectWizardContent.h"

class QtProjectWizardContentVS : public QtProjectWizardContent {
  Q_OBJECT

public:
  QtProjectWizardContentVS(QtProjectWizardWindow* window);

  virtual void populate(QGridLayout* layout, int& row) override;

private slots:
  void handleVSCDBClicked();
};
