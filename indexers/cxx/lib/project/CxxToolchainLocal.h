#pragma once
#include "project/ICxxToolchain.h"

/** The C/C++ toolchain of a process that has Clang linked in -- the indexer worker. */
class CxxToolchainLocal final : public ICxxToolchain {
public:
  [[nodiscard]] std::optional<std::vector<CxxCompileCommand>> loadCompilationDatabase(const FilePath& cdbPath,
                                                                                      std::string* error) const override;

  [[nodiscard]] std::shared_ptr<IntermediateStorage> buildPrecompiledHeader(
      const FilePath& pchInputFilePath,
      const FilePath& pchOutputFilePath,
      const std::vector<std::wstring>& compilerFlags) const override;
};
