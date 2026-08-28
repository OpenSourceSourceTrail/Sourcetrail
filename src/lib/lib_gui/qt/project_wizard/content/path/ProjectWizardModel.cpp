#include "qt/project_wizard/content/path/ProjectWizardModel.hpp"

#include "CompilationDatabaseInfo.h"
#include "FilePathFilter.h"
#include "qt/project_wizard/content/paths/QtProjectWizardContentPathsIndexedHeaders.h"
#include "settings/source_group/type/SourceGroupSettingsCxxCdb.h"
#include "utility.h"
#include "utilityFile.h"


ProjectWizardModel::ProjectWizardModel(std::shared_ptr<SourceGroupSettingsCxxCdb> settings) noexcept
    : m_settings{std::move(settings)}, m_filePaths([&]() {
      // Same filtering SourceGroupCxxCdb applies before indexing, so the count shown here is the
      // count that will be indexed. Reading the database itself is the engine's job.
      const std::vector<FilePathFilter> excludeFilters = m_settings->getExcludeFiltersExpandedAndAbsolute();
      std::set<FilePath> sourceFilePaths;
      for(const FilePath& path :
          client::inspectCompilationDatabase(m_settings->getCompilationDatabasePathExpandedAndAbsolute()).sourceFiles) {
        if(!FilePathFilter::areMatching(excludeFilters, path) && path.exists()) {
          sourceFilePaths.insert(path);
        }
      }
      return utility::getAsRelativeIfShorter(utility::toVector(sourceFilePaths), m_settings->getProjectDirectoryPath());
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
    if(client::inspectCompilationDatabase(cdbPath).valid) {
      pickedPath(window);
    }
  }
}
