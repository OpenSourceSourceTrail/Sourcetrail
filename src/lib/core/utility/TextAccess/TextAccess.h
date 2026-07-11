#pragma once
#include <memory>
#include <string>
#include <vector>

#include "FilePath.h"

class TextAccess final {
public:
  static std::shared_ptr<TextAccess> createFromFile(const FilePath& filePath);
  static std::shared_ptr<TextAccess> createFromString(const std::string& text, const FilePath& filePath = FilePath());
  static std::shared_ptr<TextAccess> createFromLines(const std::vector<std::string>& lines, const FilePath& filePath = FilePath());

  TextAccess(const TextAccess&) = delete;
  TextAccess& operator=(const TextAccess&) = delete;

  ~TextAccess();

  [[nodiscard]] uint32_t getLineCount() const;

  [[nodiscard]] bool isEmpty() const;

  [[nodiscard]] FilePath getFilePath() const;

  /**
   * @param lineNumber: starts with 1
   */
  [[nodiscard]] std::string getLine(const uint32_t lineNumber) const;

  /**
   * @param firstLineNumber: starts with 1
   * @param lastLineNumber: starts with 1
   */
  std::vector<std::string> getLines(const uint32_t firstLineNumber, const uint32_t lastLineNumber);

  const std::vector<std::string>& getAllLines() const;

  [[nodiscard]] std::string getText() const;

private:
  static std::vector<std::string> readFile(const FilePath& filePath);
  static std::vector<std::string> splitStringByLines(const std::string& text);

  TextAccess();

  bool checkIndexInRange(const uint32_t index) const;
  bool checkIndexIntervalInRange(const uint32_t firstIndex, const uint32_t lastIndex) const;

  FilePath m_filePath;
  std::vector<std::string> m_lines;
};