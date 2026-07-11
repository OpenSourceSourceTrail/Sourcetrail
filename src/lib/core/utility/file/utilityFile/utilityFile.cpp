#include "utilityFile.h"

#include <algorithm>

#include "FilePath.h"
#include "FileSystem.h"
#include "logging.h"
#include "utility.h"

std::vector<FilePath> utility::partitionFilePathsBySize(const std::vector<FilePath>& filePaths, int partitionCount) {
  using PairType = std::pair<unsigned long long, FilePath>;
  std::vector<PairType> sourceFileSizesToCommands;
  for(const FilePath& path : filePaths) {
    if(path.exists()) {
      sourceFileSizesToCommands.emplace_back(FileSystem::getFileByteSize(path), path);
    } else {
      sourceFileSizesToCommands.emplace_back(1, path);
    }
  }

  std::ranges::sort(
      sourceFileSizesToCommands, [](const PairType& item0, const PairType& item1) { return item0.first > item1.first; });

  const auto partitionCountSize = static_cast<size_t>(partitionCount);
  if(0 < partitionCountSize && partitionCountSize < sourceFileSizesToCommands.size()) {
    for(size_t index = 0; index < static_cast<size_t>(partitionCount); ++index) {
      std::sort(
          sourceFileSizesToCommands.begin() + static_cast<long>(sourceFileSizesToCommands.size() * index / partitionCountSize),
          sourceFileSizesToCommands.begin() + static_cast<long>(sourceFileSizesToCommands.size() * (index + 1) / partitionCountSize),
          [](const PairType& item0, const PairType& item1) { return item0.second.wstr() < item1.second.wstr(); });
    }
  }

  std::vector<FilePath> sortedFilePaths;
  sortedFilePaths.reserve(sourceFileSizesToCommands.size());
  for(const PairType& pair : sourceFileSizesToCommands) {
    sortedFilePaths.push_back(pair.second);
  }
  return sortedFilePaths;
}

std::vector<FilePath> utility::getTopLevelPaths(const std::vector<FilePath>& paths) {
  return utility::getTopLevelPaths(utility::toSet(paths));
}

std::vector<FilePath> utility::getTopLevelPaths(const std::set<FilePath>& paths) {
  // this works because the set contains the paths already in alphabetical order
  std::vector<FilePath> topLevelPaths;

  FilePath lastPath;
  for(const FilePath& path : paths) {
    if(lastPath.empty() || !lastPath.contains(path))    // don't add subdirectories of already added paths
    {
      lastPath = path;
      topLevelPaths.push_back(path);
    }
  }

  return topLevelPaths;
}

FilePath utility::getExpandedPath(const FilePath& path) {
  std::vector<FilePath> paths = path.expandEnvironmentVariables();
  if(!paths.empty()) {
    if(paths.size() > 1) {
      LOG_WARNING(L"Environment variable in path \"" + path.wstr() + L"\" has been expanded to " + std::to_wstring(paths.size()) +
                  L"paths, but only \"" + paths.front().wstr() + L"\" will be used.");
    }
    return paths.front();
  }
  return {};
}

std::vector<FilePath> utility::getExpandedPaths(const std::vector<FilePath>& paths) {
  std::vector<FilePath> expandedPaths;
  for(const FilePath& path : paths) {
    utility::append(expandedPaths, path.expandEnvironmentVariables());
  }
  return expandedPaths;
}

std::vector<std::filesystem::path> utility::getExpandedPaths(const std::vector<std::filesystem::path>& paths) {
  std::vector<std::filesystem::path> expandedPaths;
  for(const std::filesystem::path& path : paths) {
    utility::append(expandedPaths, FilePath{path.wstring()}.expandEnvironmentVariablesStl());
  }
  return expandedPaths;
}

FilePath utility::getExpandedAndAbsolutePath(const FilePath& path, const FilePath& baseDirectory) {
  FilePath expandedPath = getExpandedPath(path);

  if(expandedPath.empty() || expandedPath.isAbsolute()) {
    return expandedPath;
  }

  return baseDirectory.getConcatenated(expandedPath).makeCanonical();
}

FilePath utility::getAsRelativeIfShorter(const FilePath& absolutePath, const FilePath& baseDirectory) {
  if(!baseDirectory.empty()) {
    const FilePath relativePath = absolutePath.getRelativeTo(baseDirectory);
    if(relativePath.wstr().size() < absolutePath.wstr().size()) {
      return relativePath;
    }
  }
  return absolutePath;
}

std::vector<FilePath> utility::getAsRelativeIfShorter(const std::vector<FilePath>& absolutePaths, const FilePath& baseDirectory) {
  return utility::convert<FilePath, FilePath>(
      absolutePaths, [&](const FilePath& path) { return getAsRelativeIfShorter(path, baseDirectory); });
}