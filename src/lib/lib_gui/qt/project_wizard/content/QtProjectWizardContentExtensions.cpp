#include "qt/project_wizard/content/QtProjectWizardContentExtensions.h"

// Qt5
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
// internal
#include "qt/element/dialog/QtStringListBox.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithSourceExtensionsC.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithSourceExtensionsCpp.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithSourceExtensionsCxx.h"
#include "settings/source_group/component/SourceGroupSettingsWithSourceExtensions.h"

QtProjectWizardContentExtensions::QtProjectWizardContentExtensions(std::shared_ptr<SourceGroupSettingsWithSourceExtensions> settings,
                                                                   QtProjectWizardWindow* window)
    : QtProjectWizardContent(window), m_settings(settings) {}

void QtProjectWizardContentExtensions::populate(QGridLayout* layout, int& row) {
  QLabel* sourceLabel = createFormLabel(QStringLiteral("Source File Extensions"));
  layout->addWidget(sourceLabel, row, QtProjectWizardWindow::FRONT_COL, Qt::AlignTop);

  QString cxxAddition("");
  if(std::dynamic_pointer_cast<SourceGroupSettingsWithSourceExtensionsC>(m_settings) ||
     std::dynamic_pointer_cast<SourceGroupSettingsWithSourceExtensionsCpp>(m_settings) ||
     std::dynamic_pointer_cast<SourceGroupSettingsWithSourceExtensionsCxx>(m_settings)) {
    cxxAddition = QStringLiteral(
        " Files with these extensions will serve as entry points for the indexer. Headers that "
        "are included by these files will be traversed on the fly.");
  }

  addHelpButton(QStringLiteral("Source File Extensions"),
                QStringLiteral("Define extensions for source files including the dot (e.g. \".cpp\").") + cxxAddition,
                layout,
                row);

  m_listBox = new QtStringListBox(this, sourceLabel->text());
  layout->addWidget(m_listBox, row, QtProjectWizardWindow::BACK_COL);
  row++;
}

void QtProjectWizardContentExtensions::load() {
  m_listBox->setStrings(m_settings->getSourceExtensions());
}

void QtProjectWizardContentExtensions::save() {
  m_settings->setSourceExtensions(m_listBox->getStrings());
}
