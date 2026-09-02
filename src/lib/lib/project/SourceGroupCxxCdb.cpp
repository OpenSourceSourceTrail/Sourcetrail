#include "project/SourceGroupCxxCdb.h"

#include <filesystem>

#include <fmt/format.h>
#include <fmt/xchar.h>

#include "app/Application.h"
#include "data/indexer/CxxIndexerCommandProvider.h"
#include "data/indexer/IndexerCommandCxx.h"
#include "logging.h"
#include "project/RefreshInfo.h"
#include "project/utilitySourceGroupCxx.h"
#include "settings/IApplicationSettings.hpp"
#include "settings/source_group/type/SourceGroupSettingsCxxCdb.h"
#include "status/messages/MessageStatus.h"
#include "TaskLambda.h"
#include "utility.h"
#include "utilityFile.h"
#include "utilityString.h"

namespace {

/** Resolves the source file an entry names, the way Clang's own database reader would. */
FilePath sourcePathOf(const CxxCompileCommand& command, const FilePath& cdbPath) {
  FilePath sourcePath = FilePath(utility::decodeFromUtf8(command.file)).makeCanonical();
  if(!sourcePath.isAbsolute()) {
    sourcePath = FilePath(utility::decodeFromUtf8(command.directory + '/' + command.file)).makeCanonical();
    if(!sourcePath.isAbsolute()) {
      sourcePath = cdbPath.getParentDirectory().getConcatenated(sourcePath).makeCanonical();
    }
  }
  return sourcePath;
}

}    // namespace

SourceGroupCxxCdb::SourceGroupCxxCdb(std::shared_ptr<SourceGroupSettingsCxxCdb> settings) : m_settings(std::move(settings)) {}

bool SourceGroupCxxCdb::prepareIndexing() {
  FilePath cdbPath = m_settings->getCompilationDatabasePathExpandedAndAbsolute();
  if(!cdbPath.empty() && !cdbPath.exists()) {
    std::wstring error =
        L"Can't refresh project. The compilation database of the project does not exist "
        L"anymore: " +
        cdbPath.wstr();
    MessageStatus(error, true).dispatch();
    Application::getInstance()->handleDialog(error, {L"Ok"});
    return false;
  }
  return true;
}

std::set<FilePath> SourceGroupCxxCdb::filterToContainedFilePaths(const std::set<FilePath>& filePaths) const {
  return SourceGroup::filterToContainedFilePaths(filePaths,
                                                 getAllSourceFilePaths(),
                                                 utility::toSet(m_settings->getIndexedHeaderPathsExpandedAndAbsolute()),
                                                 m_settings->getExcludeFiltersExpandedAndAbsolute());
}

std::set<FilePath> SourceGroupCxxCdb::getAllSourceFilePaths() const {
  const std::optional<std::vector<CxxCompileCommand>> commands = utility::loadCompilationDatabase(
      m_settings->getCompilationDatabasePathExpandedAndAbsolute());
  return commands ? getAllSourceFilePaths(*commands) : std::set<FilePath>{};
}

std::set<FilePath> SourceGroupCxxCdb::getAllSourceFilePaths(const std::vector<CxxCompileCommand>& commands) const {
  const std::vector<FilePathFilter> excludeFilters = m_settings->getExcludeFiltersExpandedAndAbsolute();

  std::set<FilePath> sourceFilePaths;
  for(const FilePath& path : utility::getSourceFilesFromCDB(commands, m_settings->getCompilationDatabasePathExpandedAndAbsolute())) {
    if(!FilePathFilter::areMatching(excludeFilters, path) && path.exists()) {
      sourceFilePaths.insert(path);
    }
  }
  return sourceFilePaths;
}

std::shared_ptr<IndexerCommandProvider> SourceGroupCxxCdb::getIndexerCommandProvider(const RefreshInfo& info) const {
  std::shared_ptr<CxxIndexerCommandProvider> provider = std::make_shared<CxxIndexerCommandProvider>();

  const FilePath cdbPath = m_settings->getCompilationDatabasePathExpandedAndAbsolute();
  const std::optional<std::vector<CxxCompileCommand>> commands = utility::loadCompilationDatabase(cdbPath);
  if(!commands) {
    return provider;
  }

  std::vector<std::wstring> compilerFlags = getBaseCompilerFlags();
  utility::append(compilerFlags, m_settings->getCompilerFlags());

  const std::vector<std::wstring> includePchFlags = utility::getIncludePchFlags(m_settings.get());

  const std::set<FilePath> indexedHeaderPaths = utility::toSet(m_settings->getIndexedHeaderPathsExpandedAndAbsolute());
  const std::set<FilePathFilter> excludeFilters = utility::toSet(m_settings->getExcludeFiltersExpandedAndAbsolute());
  const std::set<FilePath> sourceFilePaths = getAllSourceFilePaths(*commands);

  for(const CxxCompileCommand& command : *commands) {
    const FilePath sourcePath = sourcePathOf(command, cdbPath);

    if(info.filesToIndex.contains(sourcePath) && sourceFilePaths.contains(sourcePath)) {
      std::vector<std::wstring> cdbFlags = utility::convert<std::string, std::wstring>(
          command.arguments, [](const std::string& str) { return utility::decodeFromUtf8(str); });
      if(cdbFlags.empty()) {
        continue;
      }

      // Convert windows flags to unix style for clang on windows
      if(std::filesystem::path{cdbFlags.front()}.filename() == L"cl.exe") {
        if(!utility::convertWindowsStyleFlagsToUnixStyleFlags(cdbFlags)) {
          fmt::print(L"RC file detected, skipping indexing. {}", cdbFlags.front());
          continue;
        }
      } else if(std::filesystem::path{cdbFlags.front()}.filename() == L"rc.exe") {
        fmt::print(L"RC file detected, skipping indexing. {}", cdbFlags.front());
        // Remove RC files from indexing
        continue;
      }

      utility::removeIncludePchFlag(cdbFlags);

      if(command.arguments.size() != cdbFlags.size()) {
        utility::append(cdbFlags, includePchFlags);
      }

      provider->addCommand(std::make_shared<IndexerCommandCxx>(sourcePath,
                                                               utility::concat(indexedHeaderPaths, {sourcePath}),
                                                               excludeFilters,
                                                               std::set<FilePathFilter>(),
                                                               FilePath(utility::decodeFromUtf8(command.directory)),
                                                               utility::concat(cdbFlags, compilerFlags)));
    }
  }

  provider->logStats();

  return provider;
}

std::vector<std::shared_ptr<IndexerCommand>> SourceGroupCxxCdb::getIndexerCommands(const RefreshInfo& info) const {
  return getIndexerCommandProvider(info)->consumeAllCommands();
}

std::shared_ptr<Task> SourceGroupCxxCdb::getPreIndexTask(std::shared_ptr<StorageProvider> storageProvider,
                                                         std::shared_ptr<DialogView> dialogView) const {
  if(m_settings->getPchInputFilePath().empty()) {
    return std::make_shared<TaskLambda>([]() {});
  }

  std::vector<std::wstring> compilerFlags;

  if(m_settings->getUseCompilerFlags()) {
    const FilePath cdbPath = m_settings->getCompilationDatabasePathExpandedAndAbsolute();
    if(const std::optional<std::vector<CxxCompileCommand>> commands = utility::loadCompilationDatabase(cdbPath)) {
      const std::set<FilePath> sourceFilePaths = getAllSourceFilePaths(*commands);
      for(const CxxCompileCommand& command : *commands) {
        const FilePath sourcePath = sourcePathOf(command, cdbPath);

        if(sourceFilePaths.contains(sourcePath) && utility::containsIncludePchFlag(command.arguments)) {
          for(const std::string& arg : command.arguments) {
            if((!compilerFlags.empty() || utility::isPrefix<std::string>("-", arg)) &&
               FilePath(arg).fileName() != sourcePath.fileName()) {
              compilerFlags.emplace_back(utility::decodeFromUtf8(arg));
            }
          }

          // This used to ask Clang to render the invocation and test it for "-x" "c++". The test was
          // `std::string::find(...)` used as a bool, so it was true unless the match landed at offset
          // zero -- impossible, the invocation starts with the compiler path. Spelling out what it
          // always did drops the last reason for this file to know about Clang.
          compilerFlags.push_back(L"-x");
          compilerFlags.push_back(L"c++");
          break;
        }
      }
    }
  }

  utility::append(compilerFlags, getBaseCompilerFlags());

  if(m_settings->getUseCompilerFlags()) {
    utility::append(compilerFlags, m_settings->getCompilerFlags());
  }

  utility::append(compilerFlags, m_settings->getPchFlags());

  return utility::createBuildPchTask(m_settings.get(), compilerFlags, storageProvider, dialogView);
}

std::shared_ptr<SourceGroupSettings> SourceGroupCxxCdb::getSourceGroupSettings() {
  return m_settings;
}

std::shared_ptr<const SourceGroupSettings> SourceGroupCxxCdb::getSourceGroupSettings() const {
  return m_settings;
}

std::vector<std::wstring> SourceGroupCxxCdb::getBaseCompilerFlags() const {
  std::vector<std::wstring> compilerFlags;

  IApplicationSettings* appSettings = IApplicationSettings::getInstanceRaw();

  utility::append(compilerFlags,
                  IndexerCommandCxx::getCompilerFlagsForSystemHeaderSearchPaths(
                      utility::concat(m_settings->getHeaderSearchPathsExpandedAndAbsolute(),
                                      utility::toFilePath(appSettings->getHeaderSearchPathsExpanded()))));

  utility::append(compilerFlags,
                  IndexerCommandCxx::getCompilerFlagsForFrameworkSearchPaths(
                      utility::concat(m_settings->getFrameworkSearchPathsExpandedAndAbsolute(),
                                      utility::toFilePath(appSettings->getFrameworkSearchPathsExpanded()))));

  return compilerFlags;
}
