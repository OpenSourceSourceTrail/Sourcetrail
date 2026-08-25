#pragma once
#include <string>
#include <vector>

#include "FilePath.h"

namespace client {

/**
 * What a compile_commands.json says, as plain data.
 *
 * `valid` is false when the file is missing, malformed, or the engine has no C/C++ language
 * package; `error` then carries a message fit to put in a dialog.
 */
struct CompilationDatabaseInfo final {
  bool valid = false;
  std::string error;
  /** Absolute canonical paths, unfiltered -- exclude filters are the caller's business. */
  std::vector<FilePath> sourceFiles;
  /** Every include path the compile commands mention: user, system and framework alike. */
  std::vector<FilePath> headerPaths;
  bool containsIncludePchFlags = false;
};

/**
 * Asks the engine to parse a compilation database.
 *
 * Parsing one needs Clang, and a client must not link a language package -- so this crosses the
 * engine boundary even though the file sits on the same disk. With no engine reachable the result
 * is simply invalid, which the wizard shows as a normal "cannot read this file" message.
 */
CompilationDatabaseInfo inspectCompilationDatabase(const FilePath& cdbPath);

}    // namespace client
