#include "ProjectWizardModel.hpp"

#include "QtProjectWizardContentPathsIndexedHeaders.h"
#include "SourceGroupCxxCdb.h"
#include "SourceGroupSettingsCxxCdb.h"
#include "utility.h"
#include "utilityFile.h"
#include "utilitySourceGroupCxx.h"


ProjectWizardModel::ProjectWizardModel(std::shared_ptr<SourceGroupSettingsCxxCdb> settings) noexcept
    : m_settings{std::move(settings)}, m_filePaths([&]() {
      return utility::getAsRelativeIfShorter(
          utility::toVector(SourceGroupCxxCdb(m_settings).getAllSourceFilePaths()), m_settings->getProjectDirectoryPath());
    }) {}

ProjectWizardModel::~ProjectWizardModel() noexcept = default;

void ProjectWizardModel::pickedPath(QtProjectWizardWindow* window) {
  window->saveContent();
  const FilePath projectPath = m_settings->getProjectDirectoryPath();

  std::set<FilePath> indexedHeaderPaths;
  for(const FilePath& path : QtProjectWizardContentPathsIndexedHeaders::getIndexedPathsDerivedFromCDB(m_settings)) {
    if(projectPath.contains(path)) {
      // the relative path is always shorter than the absolute path
      indexedHeaderPaths.insert(path.getRelativeTo(projectPath));
    }
  }
  m_settings->setIndexedHeaderPaths(utility::toVector(indexedHeaderPaths));
  window->loadContent();
}

void ProjectWizardModel::onPickerTextChanged(QtProjectWizardWindow* window, const QString& text) {
  const FilePath cdbPath = utility::getExpandedAndAbsolutePath(
      FilePath(text.toStdWString()), m_settings->getProjectDirectoryPath());
  if(!cdbPath.empty() && cdbPath.exists() && cdbPath != m_settings->getCompilationDatabasePathExpandedAndAbsolute()) {
    std::string error;
    std::shared_ptr<clang::tooling::JSONCompilationDatabase> cdb = utility::loadCDB(cdbPath, &error);
    if(cdb && error.empty()) {
      pickedPath(window);
    }
  }
}