#pragma once
#include <string>
#include <vector>

#include "IndexerCommand.h"

class FilePath;
namespace clang::tooling {
class JSONCompilationDatabase;
}    // namespace clang::tooling

class IndexerCommandCxx : public IndexerCommand {
public:
  static std::vector<FilePath> getSourceFilesFromCDB(const FilePath& cdbPath);
  static std::vector<FilePath> getSourceFilesFromCDB(const std::shared_ptr<clang::tooling::JSONCompilationDatabase>& cdb,
                                                     const FilePath& cdbPath);

  static std::wstring getCompilerFlagLanguageStandard(const std::wstring& languageStandard);
  static std::vector<std::wstring> getCompilerFlagsForSystemHeaderSearchPaths(const std::vector<FilePath>& systemHeaderSearchPaths);
  static std::vector<std::wstring> getCompilerFlagsForFrameworkSearchPaths(const std::vector<FilePath>& frameworkSearchPaths);

  static IndexerCommandType getStaticIndexerCommandType();

  IndexerCommandCxx(const FilePath& sourceFilePath,
                    const std::set<FilePath>& indexedPaths,
                    const std::set<FilePathFilter>& excludeFilters,
                    const std::set<FilePathFilter>& includeFilters,
                    FilePath workingDirectory,
                    const std::vector<std::wstring>& compilerFlags);

  [[nodiscard]] IndexerCommandType getIndexerCommandType() const override;
  [[nodiscard]] size_t getByteSize(size_t stringSize) const override;

  [[nodiscard]] const std::set<FilePath>& getIndexedPaths() const;
  [[nodiscard]] const std::set<FilePathFilter>& getExcludeFilters() const;
  [[nodiscard]] const std::set<FilePathFilter>& getIncludeFilters() const;
  [[nodiscard]] const std::vector<std::wstring>& getCompilerFlags() const;
  [[nodiscard]] const FilePath& getWorkingDirectory() const;

protected:
  [[nodiscard]] QJsonObject doSerialize() const override;

private:
  std::set<FilePath> mIndexedPaths;
  std::set<FilePathFilter> mExcludeFilters;
  std::set<FilePathFilter> mIncludeFilters;
  FilePath mWorkingDirectory;
  std::vector<std::wstring> mCompilerFlags;
};
