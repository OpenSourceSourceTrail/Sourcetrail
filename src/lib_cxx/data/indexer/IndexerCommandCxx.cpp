#include "IndexerCommandCxx.h"

#include <QJsonArray>
#include <QJsonObject>

#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/JSONCompilationDatabase.h>

#include "logging.h"
#include "OrderedCache.h"
#include "ResourcePaths.h"
#include "type/MessageStatus.h"
#include "utility.h"
#include "utilitySourceGroupCxx.h"
#include "utilityString.h"

std::vector<FilePath> IndexerCommandCxx::getSourceFilesFromCDB(const FilePath& cdbPath) {
  std::string error;
  const std::shared_ptr<clang::tooling::JSONCompilationDatabase> cdb = utility::loadCDB(cdbPath, &error);

  if(!error.empty()) {
    const auto message = fmt::format(
        L"Loading Clang compilation database failed with error: \"{}\"", utility::decodeFromUtf8(error));
    LOG_ERROR(message);
    MessageStatus(message, true).dispatch();
  }

  return getSourceFilesFromCDB(cdb, cdbPath);
}

std::vector<FilePath> IndexerCommandCxx::getSourceFilesFromCDB(const std::shared_ptr<clang::tooling::JSONCompilationDatabase>& cdb,
                                                               const FilePath& cdbPath) {
  std::vector<FilePath> filePaths;
  if(cdb) {
    OrderedCache<FilePath, FilePath> canonicalDirectoryPathCache([](const FilePath& path) { return path.getCanonical(); });

    for(const std::string& fileString : cdb->getAllFiles()) {
      FilePath path = FilePath(utility::decodeFromUtf8(fileString));
      if(!path.isAbsolute()) {
        std::vector<clang::tooling::CompileCommand> commands = cdb->getCompileCommands(fileString);
        if(!commands.empty()) {
          path = FilePath(utility::decodeFromUtf8(commands.front().Directory + '/' + commands.front().Filename)).makeCanonical();
        }
      }
      if(!path.isAbsolute()) {
        path = cdbPath.getParentDirectory().getConcatenated(path).makeCanonical();
      }
      filePaths.push_back(canonicalDirectoryPathCache.getValue(path.getParentDirectory()).concatenate(path.fileName()));
    }
  }
  return filePaths;
}

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

QJsonObject IndexerCommandCxx::doSerialize() const {
  QJsonObject jsonObject = IndexerCommand::doSerialize();

  {
    QJsonArray indexedPathsArray;
    for(const FilePath& indexedPath : mIndexedPaths) {
      indexedPathsArray.append(QString::fromStdWString(indexedPath.wstr()));
    }
    jsonObject["indexed_paths"] = indexedPathsArray;
  }
  {
    QJsonArray excludeFiltersArray;
    for(const FilePathFilter& excludeFilter : mExcludeFilters) {
      excludeFiltersArray.append(QString::fromStdWString(excludeFilter.wstr()));
    }
    jsonObject["exclude_filters"] = excludeFiltersArray;
  }
  {
    QJsonArray includeFiltersArray;
    for(const FilePathFilter& includeFilter : mIncludeFilters) {
      includeFiltersArray.append(QString::fromStdWString(includeFilter.wstr()));
    }
    jsonObject["include_filters"] = includeFiltersArray;
  }
  { jsonObject["working_directory"] = QString::fromStdWString(getWorkingDirectory().wstr()); }
  {
    QJsonArray compilerFlagsArray;
    for(const std::wstring& compilerFlag : mCompilerFlags) {
      compilerFlagsArray.append(QString::fromStdWString(compilerFlag));
    }
    jsonObject["compiler_flags"] = compilerFlagsArray;
  }

  return jsonObject;
}
