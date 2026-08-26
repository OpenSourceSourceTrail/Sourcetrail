#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "FilePath.h"

class IntermediateStorage;

/** One entry of a compilation database, kept exactly as the file spelled it. */
struct CxxCompileCommand {
  std::string directory;
  std::string file;
  std::vector<std::string> arguments;
};

/**
 * The two things a C/C++ source group cannot do without Clang.
 *
 * Everything else about one -- which files it covers, which flags each file is indexed with -- is
 * plain data handling and lives in lib, so the engine can build and refresh a C/C++ project with no
 * language package linked. These two are genuinely Clang: parsing a compilation database, and
 * compiling a precompiled header. They sit behind this interface so the process that has Clang
 * compiled in can provide them and the process that does not can obtain them across a boundary.
 *
 * No instance means no C/C++ toolchain in this process. That is not an error: a compilation database
 * then reads as unreadable and precompiled headers are skipped, which is exactly how the source
 * groups already behave when a database is missing.
 */
class ICxxToolchain {
public:
  virtual ~ICxxToolchain();

  /** Parses a compilation database. nullopt means unreadable; `error` then says why. */
  [[nodiscard]] virtual std::optional<std::vector<CxxCompileCommand>> loadCompilationDatabase(const FilePath& cdbPath,
                                                                                             std::string* error) const = 0;

  /** Compiles `pchInputFilePath` into `pchOutputFilePath`, returning what it indexed on the way. */
  [[nodiscard]] virtual std::shared_ptr<IntermediateStorage> buildPrecompiledHeader(
      const FilePath& pchInputFilePath, const FilePath& pchOutputFilePath, const std::vector<std::wstring>& compilerFlags) const = 0;

  /** Null when this process has no C/C++ toolchain. */
  static ICxxToolchain* getInstance();
  static void setInstance(std::shared_ptr<ICxxToolchain> toolchain);

private:
  static std::shared_ptr<ICxxToolchain> sInstance;
};
