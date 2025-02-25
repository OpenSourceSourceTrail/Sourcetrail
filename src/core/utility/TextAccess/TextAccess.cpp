#include "TextAccess.h"

#include <fstream>

#include "logging.h"

namespace {
std::istream& safeGetline(std::istream& istream, std::string& text) {
  text.clear();

  // The characters in the stream are read one-by-one using a std::streambuf.
  // That is faster than reading them one-by-one using the std::istream.
  // Code that uses streambuf this way must be guarded by a sentry object.
  // The sentry object performs various tasks,
  // such as thread synchronization and updating the stream state.

  std::istream::sentry sentry(istream, true);
  std::streambuf* pStreambuf = istream.rdbuf();

  while(true) {
    const int character = pStreambuf->sbumpc();
    switch(character) {
    case '\n':
      return istream;
    case '\r':
      if(pStreambuf->sgetc() == '\n') {
        pStreambuf->sbumpc();
      }
      return istream;
    case std::streambuf::traits_type::eof():
      // Also handle the case when the last line has no line ending
      if(text.empty()) {
        istream.setstate(std::ios::eofbit);
      }
      return istream;
    default:
      text += static_cast<char>(character);
    }
  }
}
}    // namespace

std::shared_ptr<TextAccess> TextAccess::createFromFile(const FilePath& filePath) {
  std::shared_ptr<TextAccess> result(new TextAccess());

  result->m_filePath = filePath;
  result->m_lines = readFile(filePath);

  return result;
}

std::shared_ptr<TextAccess> TextAccess::createFromString(const std::string& text, const FilePath& filePath) {
  std::shared_ptr<TextAccess> result(new TextAccess());

  result->m_lines = splitStringByLines(text);
  result->m_filePath = filePath;

  return result;
}

std::shared_ptr<TextAccess> TextAccess::createFromLines(const std::vector<std::string>& lines, const FilePath& filePath) {
  std::shared_ptr<TextAccess> result(new TextAccess());

  result->m_lines = lines;
  result->m_filePath = filePath;

  return result;
}

TextAccess::~TextAccess() = default;

uint32_t TextAccess::getLineCount() const {
  return static_cast<uint32_t>(m_lines.size());
}

bool TextAccess::isEmpty() const {
  return m_lines.empty();
}

FilePath TextAccess::getFilePath() const {
  return m_filePath;
}

std::string TextAccess::getLine(const uint32_t lineNumber) const {
  if(!checkIndexInRange(lineNumber)) {
    return "";
  }

  return m_lines[lineNumber - 1];    // -1 to correct for use as index
}

std::vector<std::string> TextAccess::getLines(const uint32_t firstLineNumber, const uint32_t lastLineNumber) {
  if(!checkIndexIntervalInRange(firstLineNumber, lastLineNumber)) {
    return {};
  }

  auto first = m_lines.begin() + firstLineNumber - 1;    // -1 to correct for use as index
  auto last = m_lines.begin() + lastLineNumber;
  return {first, last};
}

const std::vector<std::string>& TextAccess::getAllLines() const {
  return m_lines;
}

std::string TextAccess::getText() const {
  std::string result;

  for(const auto& m_line : m_lines) {
    result += m_line;
  }

  return result;
}

std::vector<std::string> TextAccess::readFile(const FilePath& filePath) {
  std::vector<std::string> result;

  try {
    std::ifstream srcFile;
    srcFile.open(filePath.str(), std::ios::binary | std::ios::in);

    if(srcFile.fail()) {
      LOG_ERROR(L"Could not open file " + filePath.wstr());
      return result;
    }

    while(!srcFile.eof()) {
      std::string line;
      safeGetline(srcFile, line);
      result.push_back(line + '\n');
    }

    srcFile.close();
  } catch(std::exception& exception) {
    LOG_ERROR(fmt::format("Exception thrown while reading file \"{}\": ", filePath.str(), exception.what()));
    result.clear();
  } catch(...) {
    LOG_ERROR(fmt::format("Unknown exception thrown while reading file \"{}\"", filePath.str()));
    result.clear();
  }

  if(!result.empty()) {
    std::string last = result.back().substr(0, result.back().size() - 1);
    result.pop_back();
    if(!last.empty()) {
      result.push_back(last);
    }
  }

  return result;
}

std::vector<std::string> TextAccess::splitStringByLines(const std::string& text) {
  std::vector<std::string> result;
  size_t prevIndex = 0;
  size_t index = text.find('\n');

  while(index != std::string::npos) {
    result.push_back(text.substr(prevIndex, index - prevIndex) + '\n');

    prevIndex = index + 1;
    index = text.find('\n', prevIndex);
  }

  if(prevIndex < text.length()) {
    result.push_back(text.substr(prevIndex));
  }

  return result;
}

TextAccess::TextAccess() = default;

bool TextAccess::checkIndexInRange(const uint32_t index) const {
  if(index < 1) {
    LOG_WARNING(fmt::format("Line numbers start with one, is ", index));
    return false;
  } else if(index > m_lines.size()) {
    LOG_WARNING(fmt::format("Tried to access index {}. Maximum index is {}", index, m_lines.size()));
    return false;
  }

  return true;
}

bool TextAccess::checkIndexIntervalInRange(const uint32_t firstIndex, const uint32_t lastIndex) const {
  if(!checkIndexInRange(firstIndex) || !checkIndexInRange(lastIndex)) {
    return false;
  } else if(firstIndex > lastIndex) {
    LOG_WARNING(fmt::format("Index 'firstLine' has to be lower or equal index 'lastLine', is {} > {}", firstIndex, lastIndex));
    return false;
  }

  return true;
}