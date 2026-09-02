#include "project/ui/content/path/ProjectWizardModel.hpp"

#include <set>

#include <QtConcurrent>

#include "CompilationDatabaseInfo.h"
#include "FilePathFilter.h"
#include "logging.h"
#include "project/ui/QtProjectWizardWindow.h"
#include "settings/source_group/type/SourceGroupSettingsCxxCdb.h"
#include "utility.h"
#include "utilityFile.h"

namespace {

/**
 * Reads the compilation database and derives everything the wizard shows from it.
 *
 * Runs on a worker thread, so it takes copies of the settings values it needs and never touches the
 * settings object itself. Reading crosses the engine boundary and stats every source file, so on a
 * large database this takes seconds -- which is why it must not run on the GUI thread.
 */
ProjectWizardModel::CdbInfo readCompilationDatabase(const FilePath& cdbPath,
                                                    const FilePath& projectPath,
                                                    const std::vector<FilePathFilter>& excludeFilters) {
  ProjectWizardModel::CdbInfo result;

  const client::CompilationDatabaseInfo info = client::inspectCompilationDatabase(cdbPath);
  if(!info.valid) {
    LOG_WARNING("Unable to read the compilation database. " + info.error);
    return result;
  }
  result.valid = true;

  // Same filtering SourceGroupCxxCdb applies before indexing, so the count shown here is the count
  // that will be indexed.
  std::set<FilePath> sourceFilePaths;
  std::set<FilePath> sourceDirectories;
  for(const FilePath& path : info.sourceFiles) {
    if(!FilePathFilter::areMatching(excludeFilters, path) && path.exists()) {
      sourceFilePaths.insert(path);
    }
    sourceDirectories.insert(path.getParentDirectory());
  }
  result.filePaths = utility::getAsRelativeIfShorter(utility::toVector(sourceFilePaths), projectPath);

  // Canonicalizing the deduplicated directories rather than every single source file: a database
  // lists thousands of files in a handful of directories, and each canonicalization is a syscall.
  std::set<FilePath> headerPaths;
  for(const FilePath& path : sourceDirectories) {
    headerPaths.insert(path.getCanonical());
  }
  for(const FilePath& path : info.headerPaths) {
    if(path.exists()) {
      headerPaths.insert(path.getCanonical());
    }
  }
  for(const FilePath& path : utility::getTopLevelPaths(headerPaths)) {
    if(path.exists() && projectPath.contains(path)) {
      // the relative path is always shorter than the absolute path
      result.indexedHeaderPaths.push_back(path.getRelativeTo(projectPath));
    }
  }
  return result;
}

}    // namespace

ProjectWizardModel::ProjectWizardModel(std::shared_ptr<SourceGroupSettingsCxxCdb> settings) noexcept
    : m_settings{std::move(settings)} {}

ProjectWizardModel::~ProjectWizardModel() noexcept = default;

void ProjectWizardModel::pickedPath(QtProjectWizardWindow* window) {
  window->saveContent();
  ensureUpToDate(window);
}

void ProjectWizardModel::onPickerTextChanged(QtProjectWizardWindow* window, const QString& text) {
  const FilePath cdbPath = utility::getExpandedAndAbsolutePath(
      FilePath(text.toStdWString()), m_settings->getProjectDirectoryPath());
  if(!cdbPath.empty() && cdbPath.exists() && cdbPath != m_settings->getCompilationDatabasePathExpandedAndAbsolute()) {
    pickedPath(window);
  }
}

void ProjectWizardModel::ensureUpToDate(QtProjectWizardWindow* window) {
  const FilePath cdbPath = m_settings->getCompilationDatabasePathExpandedAndAbsolute();
  if(cdbPath.empty() || cdbPath == m_requestedPath) {
    return;
  }
  m_requestedPath = cdbPath;

  // ponytail: a read started for an older path keeps running, only the last one is watched and its
  // result is dropped. Cancel it properly if that ever costs more than a background thread.
  QObject::connect(
      &m_watcher,
      &QFutureWatcher<CdbInfo>::finished,
      window,
      [this, window] {
        const CdbInfo info = m_watcher.result();
        m_filePaths = info.filePaths;
        if(info.valid) {
          m_settings->setIndexedHeaderPaths(info.indexedHeaderPaths);
        }
        window->loadContent();
      },
      Qt::SingleShotConnection);

  m_watcher.setFuture(QtConcurrent::run(
      readCompilationDatabase, cdbPath, m_settings->getProjectDirectoryPath(), m_settings->getExcludeFiltersExpandedAndAbsolute()));
}
