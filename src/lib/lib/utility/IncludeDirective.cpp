#include "IncludeDirective.h"

#include <utility>

IncludeDirective::IncludeDirective(FilePath includedFilePath, FilePath includingFilePath, uint32_t lineNumber, bool usesBrackets)
    : m_includedFilePath(std::move(includedFilePath))
    , m_includingFilePath(std::move(includingFilePath))
    , m_lineNumber(lineNumber)
    , m_usesBrackets(usesBrackets) {}

FilePath IncludeDirective::getIncludedFile() const {
  return m_includedFilePath;
}

FilePath IncludeDirective::getIncludingFile() const {
  return m_includingFilePath;
}

std::wstring IncludeDirective::getDirective() const {
  if(m_includedFilePath.empty()) {
    return {};
  }
  return std::wstring(L"#include ") + (m_usesBrackets ? L"<" : L"\"") + m_includedFilePath.wstr() + (m_usesBrackets ? L">" : L"\"");
}

uint32_t IncludeDirective::getLineNumber() const {
  return m_lineNumber;
}
