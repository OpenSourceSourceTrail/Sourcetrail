#include "QtProjectWizardContentPathCDB.h"
// Qt5
#include <utility>

#include <QGridLayout>
#include <QLabel>
// internal
#include "ProjectWizardModel.hpp"
#include "QtProjectWizardContentPathsIndexedHeaders.h"
#include "SourceGroupCxxCdb.h"
#include "SourceGroupSettingsCxxCdb.h"
#include "utility.h"
#include "utilityFile.h"
#include "utilitySourceGroupCxx.h"

QtProjectWizardContentPathCDB::QtProjectWizardContentPathCDB(std::shared_ptr<ProjectWizardModel> model,
                                                             QtProjectWizardWindow* window)
    : QtProjectWizardContentPath(window), m_model(std::move(model)) {
  setObjectName("QtProjectWizardContentPathCDB");
  setTitleString(QStringLiteral("Compilation Database (compile_commands.json)"));
  setHelpString(
      "Select the compilation database file for the project. Sourcetrail will index your project "
      "based on the compile "
      "commands. This file contains using all include paths and compiler flags of these compile "
      "commands. The project "
      "will stay up to date with changes in the compilation database on every refresh.<br />"
      "<br />"
      "You can make use of environment variables with ${ENV_VAR}.");
  setFileEndings({L".json"});
  setIsRequired(true);
}

QtProjectWizardContentPathCDB::~QtProjectWizardContentPathCDB() = default;

void QtProjectWizardContentPathCDB::populate(QGridLayout* layout, int& row) {
  QtProjectWizardContentPath::populate(layout, row);
  m_picker->setPickDirectory(false);
  m_picker->setFileFilter(QStringLiteral("JSON Compilation Database (*.json)"));

  connect(m_picker, &QtLocationPicker::locationPicked, [this]() { m_model->pickedPath(mWindow); });
  connect(m_picker, &QtLocationPicker::textChanged, [this](const QString& text) { m_model->onPickerTextChanged(mWindow, text); });

  auto* description = new QLabel(
      "Sourcetrail will use all include paths and compiler flags from the Compilation Database "
      "and stay up-to-date "
      "with changes on refresh.",
      this);
  description->setObjectName(QStringLiteral("description"));
  description->setWordWrap(true);
  layout->addWidget(description, row, QtProjectWizardWindow::BACK_COL);
  row++;

  QLabel* title = createFormSubLabel(QStringLiteral("Source Files to Index"));
  layout->addWidget(title, row, QtProjectWizardWindow::FRONT_COL, Qt::AlignTop);
  layout->setRowStretch(row, 0);

  m_fileCountLabel = new QLabel(QLatin1String(""));
  m_fileCountLabel->setWordWrap(true);
  layout->addWidget(m_fileCountLabel, row, QtProjectWizardWindow::BACK_COL, Qt::AlignTop);
  row++;

  addFilesButton(QStringLiteral("show source files"), layout, row);
  row++;
}

void QtProjectWizardContentPathCDB::load() {
  m_picker->setText(QString::fromStdWString(m_model->settings()->getCompilationDatabasePath().wstr()));

  refresh();
}

void QtProjectWizardContentPathCDB::save() {
  m_model->settings()->setCompilationDatabasePath(FilePath(m_picker->getText().toStdWString()));
}

void QtProjectWizardContentPathCDB::refresh() {
  m_model->clearFilePaths();

  if(m_fileCountLabel != nullptr) {
    m_fileCountLabel->setText("<b>" + QString::number(getFilePaths().size()) +
                              "</b> source files were found in the compilation database.");
  }
}

std::vector<FilePath> QtProjectWizardContentPathCDB::getFilePaths() const {
  return m_model->filePaths();
}

QString QtProjectWizardContentPathCDB::getFileNamesTitle() const {
  return QStringLiteral("Source Files");
}

QString QtProjectWizardContentPathCDB::getFileNamesDescription() const {
  return QStringLiteral(" source files will be indexed.");
}

std::shared_ptr<SourceGroupSettings> QtProjectWizardContentPathCDB::getSourceGroupSettings() {
  return m_model->settings();
}