#include "FileManager.h"

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>

#include "FilePath.h"
#include "FilePathFilter.h"
#include "FileSystem.h"

FileManager::FileManager() = default;

FileManager::~FileManager() = default;

void FileManager::update(std::vector<FilePath> sourcePaths,
                         std::vector<FilePathFilter> excludeFilters,
                         std::vector<std::wstring> sourceExtensions) {
  m_sourcePaths = std::move(sourcePaths);
  m_excludeFilters = std::move(excludeFilters);
  m_sourceExtensions = std::move(sourceExtensions);

  m_allSourceFilePaths.clear();

  const auto filterFunc = [this](const FileInfo& fileInfo) -> bool {
    const FilePath& filePath = fileInfo.path;
    return !isExcluded(filePath);
  };
  const auto transformFunc = [](const FileInfo& fileInfo) -> FilePath { return fileInfo.path; };

  const auto files = FileSystem::getFileInfosFromPaths(m_sourcePaths, m_sourceExtensions);
  m_allSourceFilePaths = files | ranges::cpp20::views::filter(filterFunc) | ranges::cpp20::views::transform(transformFunc) |
      ranges::to<std::set>();
}

std::vector<FilePath> FileManager::getSourcePaths() const {
  return m_sourcePaths;
}

bool FileManager::hasSourceFilePath(const FilePath& filePath) const {
  return m_allSourceFilePaths.find(filePath) != m_allSourceFilePaths.end();
}

std::set<FilePath> FileManager::getAllSourceFilePaths() const {
  return m_allSourceFilePaths;
}

bool FileManager::isExcluded(const FilePath& filePath) const {
  return FilePathFilter::areMatching(m_excludeFilters, filePath);
}
