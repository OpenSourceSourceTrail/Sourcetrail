#pragma once

#include "FilePath.h"

class IncludeDirective final {
public:
  IncludeDirective(FilePath includedFilePath, FilePath includingFilePath, uint32_t lineNumber, bool usesBrackets);

  FilePath getIncludedFile() const;
  FilePath getIncludingFile() const;
  std::wstring getDirective() const;
  uint32_t getLineNumber() const;

private:
  FilePath m_includedFilePath;
  FilePath m_includingFilePath;
  uint32_t m_lineNumber;
  bool m_usesBrackets;
};