#pragma once
// Qt5
#include <QGridLayout>
#include <QLabel>
// internal
#include "qt/project_wizard/content/QtProjectWizardContent.h"

class QtProjectWizardContentRequiredLabel : public QtProjectWizardContent {
public:
  QtProjectWizardContentRequiredLabel(QtProjectWizardWindow* window) : QtProjectWizardContent(window) {}

  // QtProjectWizardContent implementation
  void populate(QGridLayout* layout, int& row) override {
    QLabel* label = createFormLabel(QStringLiteral("* required"));
    layout->addWidget(label, row, QtProjectWizardWindow::FRONT_COL, Qt::AlignTop);
    row++;
  }
};
