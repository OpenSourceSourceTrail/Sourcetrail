#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ICxxToolchain.h"

class DialogView;
class FilePath;
class SourceGroupSettingsWithCxxPchOptions;
class StorageProvider;
class Task;

namespace utility {
/** Builds the precompiled header the settings ask for; a no-op task when they ask for none. */
std::shared_ptr<Task> createBuildPchTask(const SourceGroupSettingsWithCxxPchOptions* settings,
                                         std::vector<std::wstring> compilerFlags,
                                         const std::shared_ptr<StorageProvider>& storageProvider,
                                         const std::shared_ptr<DialogView>& dialogView);

/** Reads a compilation database through the registered toolchain; nullopt when there is none. */
std::optional<std::vector<CxxCompileCommand>> loadCompilationDatabase(const FilePath& cdbPath, std::string* error = nullptr);

/** Lists the source files a compilation database names, as absolute canonical paths. */
std::vector<FilePath> getSourceFilesFromCDB(const FilePath& cdbPath);
std::vector<FilePath> getSourceFilesFromCDB(const std::vector<CxxCompileCommand>& commands, const FilePath& cdbPath);

bool containsIncludePchFlags(const std::vector<CxxCompileCommand>& commands);
bool containsIncludePchFlag(const std::vector<std::string>& args);
std::vector<std::wstring> getWithRemoveIncludePchFlag(const std::vector<std::wstring>& args);

/** Converts Windows-style compiler flags to Unix-style flags in-place. */
bool convertWindowsStyleFlagsToUnixStyleFlags(std::vector<std::wstring>& args);

void removeIncludePchFlag(std::vector<std::wstring>& args);
std::vector<std::wstring> getIncludePchFlags(const SourceGroupSettingsWithCxxPchOptions* settings);
}    // namespace utility
