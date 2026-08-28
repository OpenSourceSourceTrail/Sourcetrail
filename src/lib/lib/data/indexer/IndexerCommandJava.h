#pragma once

#include <set>

#include "data/indexer/IndexerCommand.h"

class IndexerCommandJava : public IndexerCommand {
public:
  static IndexerCommandType getStaticIndexerCommandType();

  IndexerCommandJava(const FilePath& sourceFilePath, std::set<FilePath> classPaths, std::wstring languageStandard);

  [[nodiscard]] IndexerCommandType getIndexerCommandType() const override;
  [[nodiscard]] size_t getByteSize(size_t stringSize) const override;

  [[nodiscard]] const std::set<FilePath>& getClassPaths() const;
  [[nodiscard]] const std::wstring& getLanguageStandard() const;

protected:
  [[nodiscard]] boost::json::object doSerialize() const override;

private:
  std::set<FilePath> m_classPaths;
  std::wstring m_languageStandard;
};
