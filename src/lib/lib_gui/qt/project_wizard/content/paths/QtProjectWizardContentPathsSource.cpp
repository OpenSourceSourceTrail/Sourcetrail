#include "QtProjectWizardContentPathsSource.h"

#include <QMessageBox>

#include "FileManager.h"
#include "FilePathFilter.h"
#include "SourceGroupSettings.h"
#include "SourceGroupSettingsWithExcludeFilters.h"
#include "SourceGroupSettingsWithSourceExtensions.h"
#include "SourceGroupSettingsWithSourcePaths.h"
#include "utility.h"
#include "utilityFile.h"

QtProjectWizardContentPathsSource::QtProjectWizardContentPathsSource(std::shared_ptr<SourceGroupSettings> settings,
                                                                     QtProjectWizardWindow* window)
    : QtProjectWizardContentPaths(settings, window, QtPathListBox::SELECTION_POLICY_FILES_AND_DIRECTORIES, true) {
  m_showFilesString = QStringLiteral("show files");

  setTitleString(QStringLiteral("Files & Directories to Index"));
  setHelpString(
      QStringLiteral("These paths define the files and directories that will be indexed by Sourcetrail. Provide "
                     "a directory to recursively "
                     "add all contained source and header files.<br />"
                     "<br />"
                     "If your project's source code resides in one location, but generated source files are "
                     "kept at a different location, "
                     "you will also need to add that directory.<br />"
                     "<br />"
                     "You can make use of environment variables with ${ENV_VAR}."));
  setIsRequired(true);
}

void QtProjectWizardContentPathsSource::load() {
  if(std::shared_ptr<SourceGroupSettingsWithSourcePaths> pathSettings =
         std::dynamic_pointer_cast<SourceGroupSettingsWithSourcePaths>(m_settings))    // FIXME: pass msettings as required type
  {
    m_list->setPaths(pathSettings->getSourcePaths());
  }
}

void QtProjectWizardContentPathsSource::save() {
  if(std::shared_ptr<SourceGroupSettingsWithSourcePaths> pathSettings =
         std::dynamic_pointer_cast<SourceGroupSettingsWithSourcePaths>(m_settings))    // FIXME: pass msettings as required type
  {
    pathSettings->setSourcePaths(m_list->getPathsAsDisplayed());
  }
}

bool QtProjectWizardContentPathsSource::check() {
  if(m_list->getPathsAsDisplayed().empty()) {
    QMessageBox msgBox(m_window);
    msgBox.setText(QStringLiteral("You didn't specify any 'Files & Directories to Index'."));
    msgBox.setInformativeText(
        QStringLiteral("Sourcetrail will not index any files for this Source Group. Please add "
                       "paths to files or directories "
                       "that should be indexed."));
    QPushButton* yesButton = msgBox.addButton(QStringLiteral("Continue"), QMessageBox::ButtonRole::YesRole);
    msgBox.addButton(QStringLiteral("Cancel"), QMessageBox::ButtonRole::NoRole);
    msgBox.setDefaultButton(yesButton);

    if(msgBox.exec() != 0) {
      return false;
    }
  }

  return QtProjectWizardContentPaths::check();
}

std::vector<FilePath> QtProjectWizardContentPathsSource::getFilePaths() const {
  // Every source group whose files come from "scan these paths for these extensions" is listed the
  // same way, whatever language it indexes. Asking the settings components rather than a concrete
  // SourceGroup is what keeps this out of the language packages.
  const auto pathSettings = std::dynamic_pointer_cast<SourceGroupSettingsWithSourcePaths>(m_settings);
  const auto extensionSettings = std::dynamic_pointer_cast<SourceGroupSettingsWithSourceExtensions>(m_settings);
  if(!pathSettings || !extensionSettings) {
    return {};
  }

  std::vector<FilePathFilter> excludeFilters;
  if(const auto filterSettings = std::dynamic_pointer_cast<SourceGroupSettingsWithExcludeFilters>(m_settings)) {
    excludeFilters = filterSettings->getExcludeFiltersExpandedAndAbsolute();
  }

  FileManager fileManager;
  fileManager.update(pathSettings->getSourcePathsExpandedAndAbsolute(), excludeFilters, extensionSettings->getSourceExtensions());

  return utility::getAsRelativeIfShorter(utility::toVector(fileManager.getAllSourceFilePaths()),
                                         m_settings->getProjectDirectoryPath());
}

QString QtProjectWizardContentPathsSource::getFileNamesTitle() const {
  return QStringLiteral("Indexed Files");
}

QString QtProjectWizardContentPathsSource::getFileNamesDescription() const {
  return QStringLiteral(" files will be indexed.");
}
