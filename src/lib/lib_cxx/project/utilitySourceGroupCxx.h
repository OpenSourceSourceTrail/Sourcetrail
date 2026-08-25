#pragma once
#include <memory>
#include <string>
#include <vector>

namespace clang::tooling {
class JSONCompilationDatabase;
}    // namespace clang::tooling

class DialogView;
class FilePath;
class SourceGroupSettingsWithCxxPchOptions;
class StorageProvider;
class Task;

namespace utility {
std::shared_ptr<Task> createBuildPchTask(const SourceGroupSettingsWithCxxPchOptions* settings,
                                         std::vector<std::wstring> compilerFlags,
                                         const std::shared_ptr<StorageProvider>& storageProvider,
                                         const std::shared_ptr<DialogView>& dialogView);

std::shared_ptr<clang::tooling::JSONCompilationDatabase> loadCDB(const FilePath& cdbPath, std::string* error = nullptr);

/** Lists the source files a compilation database names, as absolute canonical paths.
 *
 * Parsing a compilation database needs Clang, which is why this lives here and not next to
 * IndexerCommandCxx: the command itself is plain data that the engine and the GUI must be able
 * to build and serialize without a language package.
 */
std::vector<FilePath> getSourceFilesFromCDB(const FilePath& cdbPath);
std::vector<FilePath> getSourceFilesFromCDB(const std::shared_ptr<clang::tooling::JSONCompilationDatabase>& cdb,
                                            const FilePath& cdbPath);

/** Lists the source files a compilation database names, as absolute canonical paths.
 *
 * Parsing a compilation database needs Clang, which is why this lives here and not next to
 * IndexerCommandCxx: the command itself is plain data that the engine and the GUI must be able
 * to build and serialize without a language package.
 */
std::vector<FilePath> getSourceFilesFromCDB(const FilePath& cdbPath);
std::vector<FilePath> getSourceFilesFromCDB(const std::shared_ptr<clang::tooling::JSONCompilationDatabase>& cdb,
                                            const FilePath& cdbPath);
bool containsIncludePchFlags(const std::shared_ptr<clang::tooling::JSONCompilationDatabase>& cdb);
bool containsIncludePchFlag(const std::vector<std::string>& args);
std::vector<std::wstring> getWithRemoveIncludePchFlag(const std::vector<std::wstring>& args);

/** Converts Windows-style compiler flags to Unix-style flags in-place. */
bool convertWindowsStyleFlagsToUnixStyleFlags(std::vector<std::wstring>& args);

void removeIncludePchFlag(std::vector<std::wstring>& args);
std::vector<std::wstring> getIncludePchFlags(const SourceGroupSettingsWithCxxPchOptions* settings);
}    // namespace utility
