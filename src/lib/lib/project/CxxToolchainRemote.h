#pragma once
#include "ICxxToolchain.h"

/**
 * The C/C++ toolchain of a process that has no Clang -- the engine and the CLI.
 *
 * Each call runs the C/C++ indexer binary once in helper mode and reads its answer back. The
 * indexer is found through its plugin manifest, the same lookup indexing itself uses, so a missing
 * or unregistered indexer simply means no toolchain: compilation databases read as unreadable and
 * precompiled headers are skipped, and an already-indexed project still opens and browses.
 */
class CxxToolchainRemote final : public ICxxToolchain {
public:
  [[nodiscard]] std::optional<std::vector<CxxCompileCommand>> loadCompilationDatabase(const FilePath& cdbPath,
                                                                                      std::string* error) const override;

  [[nodiscard]] std::shared_ptr<IntermediateStorage> buildPrecompiledHeader(
      const FilePath& pchInputFilePath,
      const FilePath& pchOutputFilePath,
      const std::vector<std::wstring>& compilerFlags) const override;
};
