#include "IndexerCommandCxx.h"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>

#include "ResourcePaths.h"
#include "utility.h"
#include "utilityString.h"

std::wstring IndexerCommandCxx::getCompilerFlagLanguageStandard(const std::wstring& languageStandard) {
  return L"-std=" + languageStandard;
}

std::vector<std::wstring> IndexerCommandCxx::getCompilerFlagsForSystemHeaderSearchPaths(
    const std::vector<FilePath>& systemHeaderSearchPaths) {
  std::vector<std::wstring> compilerFlags;
  compilerFlags.reserve(systemHeaderSearchPaths.size() * 2);

  for(const FilePath& path : systemHeaderSearchPaths) {
    compilerFlags.emplace_back(L"-isystem");
    compilerFlags.push_back(path.wstr());
  }

#ifdef _WIN32
  // prepend clang system includes on windows
  compilerFlags = utility::concat({L"-isystem", ResourcePaths::getCxxCompilerHeaderDirectoryPath().wstr()}, compilerFlags);
#else
  // append otherwise
  compilerFlags.emplace_back(L"-isystem");
  compilerFlags.push_back(ResourcePaths::getCxxCompilerHeaderDirectoryPath().wstr());
#endif

  return compilerFlags;
}

std::vector<std::wstring> IndexerCommandCxx::getCompilerFlagsForFrameworkSearchPaths(
    const std::vector<FilePath>& frameworkSearchPaths) {
  std::vector<std::wstring> compilerFlags;
  compilerFlags.reserve(frameworkSearchPaths.size() * 2);
  for(const FilePath& path : frameworkSearchPaths) {
    compilerFlags.emplace_back(L"-iframework");
    compilerFlags.push_back(path.wstr());
  }
  return compilerFlags;
}

IndexerCommandType IndexerCommandCxx::getStaticIndexerCommandType() {
  return INDEXER_COMMAND_CXX;
}
IndexerCommandCxx::IndexerCommandCxx(const FilePath& sourceFilePath,
                                     const std::set<FilePath>& indexedPaths,
                                     const std::set<FilePathFilter>& excludeFilters,
                                     const std::set<FilePathFilter>& includeFilters,
                                     FilePath workingDirectory,
                                     const std::vector<std::wstring>& compilerFlags)
    : IndexerCommand(sourceFilePath)
    , mIndexedPaths(indexedPaths)
    , mExcludeFilters(excludeFilters)
    , mIncludeFilters(includeFilters)
    , mWorkingDirectory(std::move(workingDirectory))
    , mCompilerFlags(compilerFlags) {}

IndexerCommandType IndexerCommandCxx::getIndexerCommandType() const {
  return getStaticIndexerCommandType();
}

size_t IndexerCommandCxx::getByteSize(size_t stringSize) const {
  size_t size = IndexerCommand::getByteSize(stringSize);

  for(const FilePath& path : mIndexedPaths) {
    size += stringSize + utility::encodeToUtf8(path.wstr()).size();
  }

  for(const FilePathFilter& filter : mExcludeFilters) {
    size += stringSize + utility::encodeToUtf8(filter.wstr()).size();
  }

  for(const FilePathFilter& filter : mIncludeFilters) {
    size += stringSize + utility::encodeToUtf8(filter.wstr()).size();
  }

  for(const std::wstring& flag : mCompilerFlags) {
    size += stringSize + flag.size();
  }

  return size;
}

const std::set<FilePath>& IndexerCommandCxx::getIndexedPaths() const {
  return mIndexedPaths;
}

const std::set<FilePathFilter>& IndexerCommandCxx::getExcludeFilters() const {
  return mExcludeFilters;
}

const std::set<FilePathFilter>& IndexerCommandCxx::getIncludeFilters() const {
  return mIncludeFilters;
}

const std::vector<std::wstring>& IndexerCommandCxx::getCompilerFlags() const {
  return mCompilerFlags;
}

const FilePath& IndexerCommandCxx::getWorkingDirectory() const {
  return mWorkingDirectory;
}

boost::json::object IndexerCommandCxx::doSerialize() const {
  boost::json::object jsonObject = IndexerCommand::doSerialize();

  {
    boost::json::array indexedPathsArray;
    for(const FilePath& indexedPath : mIndexedPaths) {
      indexedPathsArray.emplace_back(utility::encodeToUtf8(indexedPath.wstr()));
    }
    jsonObject["indexed_paths"] = std::move(indexedPathsArray);
  }
  {
    boost::json::array excludeFiltersArray;
    for(const FilePathFilter& excludeFilter : mExcludeFilters) {
      excludeFiltersArray.emplace_back(utility::encodeToUtf8(excludeFilter.wstr()));
    }
    jsonObject["exclude_filters"] = std::move(excludeFiltersArray);
  }
  {
    boost::json::array includeFiltersArray;
    for(const FilePathFilter& includeFilter : mIncludeFilters) {
      includeFiltersArray.emplace_back(utility::encodeToUtf8(includeFilter.wstr()));
    }
    jsonObject["include_filters"] = std::move(includeFiltersArray);
  }
  jsonObject["working_directory"] = utility::encodeToUtf8(getWorkingDirectory().wstr());
  {
    boost::json::array compilerFlagsArray;
    for(const std::wstring& compilerFlag : mCompilerFlags) {
      compilerFlagsArray.emplace_back(utility::encodeToUtf8(compilerFlag));
    }
    jsonObject["compiler_flags"] = std::move(compilerFlagsArray);
  }

  return jsonObject;
}
