#pragma once
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "project/logic/ICxxToolchain.h"

class DialogView;
class FilePath;
class FilePathFilter;
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

/**
 * The source files worth indexing out of the ones a compilation database names.
 *
 * A database lists what the build compiles, which is not the same as what exists on disk today, and
 * it knows nothing about the project's exclude filters.
 */
std::set<FilePath> filterCdbSourceFiles(const std::vector<FilePath>& sourceFiles,
                                        const std::vector<FilePathFilter>& excludeFilters);

/**
 * The top-level directories that could hold a compilation database's headers.
 *
 * Derived from the directories of *every* source file the database names -- excluding a source file
 * from indexing says nothing about the headers beside it -- plus the include paths the compile
 * commands mention. Paths that do not exist are dropped and nested ones are absorbed by their
 * parent, so the result is the shortest set that still covers everything.
 */
std::vector<FilePath> deriveCdbHeaderRoots(const std::vector<FilePath>& sourceFiles, const std::vector<FilePath>& headerPaths);

bool containsIncludePchFlags(const std::vector<CxxCompileCommand>& commands);
bool containsIncludePchFlag(const std::vector<std::string>& args);
std::vector<std::wstring> getWithRemoveIncludePchFlag(const std::vector<std::wstring>& args);

/** Converts Windows-style compiler flags to Unix-style flags in-place. */
bool convertWindowsStyleFlagsToUnixStyleFlags(std::vector<std::wstring>& args);

/**
 * The angle-bracket includes worth putting into an automatic precompiled header.
 *
 * A textual scan, not a real dependency scan: it reads the head of every source file and keeps the
 * `#include <...>` lines enough of them share. Angle brackets are the filter that matters -- they
 * are how a project reaches headers it does not own -- and anything that still resolves inside the
 * indexed paths is dropped, because precompiling a header hides it from the preprocessor callbacks
 * the indexer records include edges and macros with.
 */
std::vector<std::string> collectAutoPchIncludes(const std::vector<FilePath>& sourceFiles,
                                                const std::vector<FilePath>& includeDirs,
                                                const std::set<FilePath>& indexedPaths);

/** Writes those includes as a prefix header; an empty path when there was nothing to write. */
FilePath writeAutoPchHeader(const std::vector<std::string>& includes,
                            const FilePath& outputDirectory,
                            const std::wstring& headerName = L"sourcetrail_auto_pch.h");

/**
 * Derives a precompiled header for files that share compiler flags and builds it.
 *
 * Returns the flags that make a parse use it, empty when there was nothing worth precompiling or
 * the build did not produce a file. The files must share their macro state: a precompiled header is
 * rejected by Clang when the command line defines a macro differently than the header recorded.
 */
std::vector<std::wstring> buildAutoPch(const std::vector<FilePath>& sourceFiles,
                                       const std::vector<std::wstring>& compilerFlags,
                                       const std::set<FilePath>& indexedPaths,
                                       const FilePath& outputDirectory,
                                       const std::wstring& name);

/** The macro state of a command line: what a precompiled header has to agree with to be usable. */
std::wstring macroSignatureOf(const std::vector<std::wstring>& compilerFlags);

void removeIncludePchFlag(std::vector<std::wstring>& args);
std::vector<std::wstring> getIncludePchFlags(const SourceGroupSettingsWithCxxPchOptions* settings);
}    // namespace utility
