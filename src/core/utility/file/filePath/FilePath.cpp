#include "FilePath.h"

#include <regex>

#include "FileSystem.h"
#include "logging.h"
#include "utilityString.h"

namespace fs = std::filesystem;

namespace {
const std::regex env("\\$\\{([^}]+)\\}|%([^%]+)%");    // ${VARIABLE_NAME} or %VARIABLE_NAME%
}    // namespace

FilePath::FilePath() : m_path(std::make_unique<fs::path>()) {}

FilePath::FilePath(const std::string& filePath) : m_path(std::make_unique<fs::path>(filePath)) {}

FilePath::FilePath(const std::wstring& filePath) : m_path(std::make_unique<fs::path>(filePath)) {}

FilePath::FilePath(const FilePath& other)
    : m_path(std::make_unique<fs::path>(other.getPath()))
    , m_exists(other.m_exists)
    , m_isDirectory(other.m_isDirectory)
    , m_canonicalized(other.m_canonicalized) {}

FilePath::FilePath(FilePath&& other) noexcept
    : m_path(std::move(other.m_path))
    , m_exists(other.m_exists)
    , m_isDirectory(other.m_isDirectory)
    , m_canonicalized(other.m_canonicalized) {}

FilePath::FilePath(const std::wstring& filePath, const std::wstring& basePath) {
  fs::path p{filePath}, base{basePath};
  if(p.is_absolute()) {
    m_path = std::make_unique<fs::path>(std::move(p));
  } else {
    std::error_code ec;
    m_path = std::make_unique<fs::path>(fs::absolute(base / p, ec));
    if(ec) {
      LOG_ERROR(ec.message());
    }
  }
}

FilePath::~FilePath() = default;

const FilePath& FilePath::EmptyFilePath() {
  static const FilePath sEmptyFilePath;
  return sEmptyFilePath;
}

fs::path FilePath::getPath() const {
  return *m_path;
}

bool FilePath::empty() const {
  return m_path->empty();
}

bool FilePath::exists() const noexcept {
  if(!m_exists.has_value()) {
    m_exists = fs::exists(getPath());
  }

  return m_exists.value();
}

bool FilePath::recheckExists() const {
  m_exists.reset();
  return exists();
}

bool FilePath::isDirectory() const noexcept {
  if(!m_isDirectory.has_value()) {
    m_isDirectory = fs::is_directory(getPath());
  }

  return m_isDirectory.value();
}

bool FilePath::isAbsolute() const {
  return m_path->is_absolute();
}

bool FilePath::isValid() const {
  fs::path::iterator it = m_path->begin();

  if(isAbsolute() && m_path->has_root_path()) {
    std::string root = m_path->root_path().string();
    std::string current = "";
    while(current.size() < root.size()) {
      current += it->string();
      it++;
    }
  }

  for(; it != m_path->end(); ++it) {
    if(!filesystem::isPortableName(it->string())) {
      return false;
    }
  }

  return true;
}

FilePath FilePath::getParentDirectory() const {
  FilePath parentDirectory(m_path->parent_path().wstring());

  if(!parentDirectory.empty()) {
    parentDirectory.m_isDirectory = true;

    if(m_exists) {
      parentDirectory.m_exists = true;
    }
  }

  return parentDirectory;
}

FilePath& FilePath::makeAbsolute() {
  std::error_code ec;
  m_path = std::make_unique<fs::path>(fs::absolute(getPath(), ec));
  if(ec) {
    LOG_ERROR(ec.message());
  }
  return *this;
}

FilePath FilePath::getAbsolute() const {
  FilePath path(*this);
  path.makeAbsolute();
  return path;
}

FilePath& FilePath::makeCanonical() {
  if(m_canonicalized || !exists()) {
    return *this;
  }

  std::error_code ec;
  auto canonicalPath = fs::canonical(getPath(), ec);
  if(ec) {
    LOG_ERROR(ec.message());
    return *this;
  }

  m_path = std::make_unique<fs::path>(std::move(canonicalPath));
  m_canonicalized = true;
  return *this;
}

FilePath FilePath::getCanonical() const {
  FilePath path(*this);
  path.makeCanonical();
  return path;
}

std::vector<FilePath> FilePath::expandEnvironmentVariables() const {
  std::vector<FilePath> paths;
  std::string text = str();

  std::smatch match;
  while(std::regex_search(text, match, env)) {
#ifdef _WIN32
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
    const char* s = match[1].matched ? getenv(match[1].str().c_str()) : getenv(match[2].str().c_str());
#ifdef _WIN32
#  pragma warning(pop)
#endif
    if(s == nullptr) {
      LOG_ERROR(match[1].str() + " is not an environment variable in: " + text);
      return paths;
    }
    text.replace(static_cast<size_t>(match.position(0)), static_cast<size_t>(match.length(0)), s);
  }

  char environmentVariablePathSeparator = ':';

#if defined(_WIN32) || defined(_WIN64)
  environmentVariablePathSeparator = ';';
#endif

  for(const std::string& str : utility::splitToVector(text, environmentVariablePathSeparator)) {
    if(!str.empty()) {
      paths.emplace_back(str);
    }
  }

  return paths;
}

std::vector<fs::path> FilePath::expandEnvironmentVariablesStl() const {
  std::vector<fs::path> paths;
  std::string text = str();

  std::smatch match;
  while(std::regex_search(text, match, env)) {
#ifdef _WIN32
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
    const char* s = match[1].matched ? getenv(match[1].str().c_str()) : getenv(match[2].str().c_str());
#ifdef _WIN32
#  pragma warning(pop)
#endif
    if(s == nullptr) {
      LOG_ERROR(match[1].str() + " is not an environment variable in: " + text);
      return paths;
    }
    text.replace(static_cast<size_t>(match.position(0)), static_cast<size_t>(match.length(0)), s);
  }

  char environmentVariablePathSeparator = ':';

#if defined(_WIN32) || defined(_WIN64)
  environmentVariablePathSeparator = ';';
#endif

  for(const std::string& str : utility::splitToVector(text, environmentVariablePathSeparator)) {
    if(!str.empty()) {
      paths.emplace_back(str);
    }
  }

  return paths;
}

FilePath& FilePath::makeRelativeTo(const FilePath& other) {
  const fs::path a = this->getCanonical().getPath();
  const fs::path b = other.getCanonical().getPath();

  if(a.root_path() != b.root_path()) {
    return *this;
  }

  auto itA = a.begin();
  auto itB = b.begin();

  while(itA != a.end() && itB != b.end() && *itA == *itB) {
    itA++;
    itB++;
  }

  fs::path r;

  if(itB != b.end()) {
    if(!fs::is_directory(b)) {
      itB++;
    }

    for(; itB != b.end(); itB++) {
      r /= "..";
    }
  }

  for(; itA != a.end(); itA++) {
    r /= *itA;
  }

  if(r.empty()) {
    r = "./";
  }

  m_path = std::make_unique<fs::path>(r);
  return *this;
}


FilePath FilePath::getRelativeTo(const FilePath& other) const {
  FilePath path(*this);
  path.makeRelativeTo(other);
  return path;
}

FilePath& FilePath::concatenate(const FilePath& other) {
  m_path->operator/=(other.getPath());
  m_exists.reset();
  m_isDirectory.reset();
  m_canonicalized = false;

  return *this;
}

FilePath FilePath::getConcatenated(const FilePath& other) const {
  FilePath path(*this);
  path.concatenate(other);
  return path;
}

FilePath& FilePath::concatenate(const std::wstring& other) {
  m_path->operator/=(other);
  m_exists.reset();
  m_isDirectory.reset();
  m_canonicalized = false;

  return *this;
}

FilePath FilePath::getConcatenated(const std::wstring& other) const {
  FilePath path(*this);
  path.concatenate(other);
  return path;
}

FilePath FilePath::getLowerCase() const {
  return FilePath(utility::toLowerCase(wstr()));
}

bool FilePath::contains(const FilePath& other) const {
  if(!isDirectory()) {
    return false;
  }

  fs::path dir = getPath();
  const std::unique_ptr<fs::path>& dir2 = other.m_path;

  if(dir.filename() == ".") {
    dir.remove_filename();
  }

  auto it = dir.begin();
  auto it2 = dir2->begin();

  while(it != dir.end()) {
    if(it2 == dir2->end()) {
      return false;
    }

    if(*it != *it2) {
      return false;
    }

    it++;
    it2++;
  }

  return true;
}

std::string FilePath::str() const {
  return m_path->generic_string();
}

std::wstring FilePath::wstr() const {
  return m_path->generic_wstring();
}

std::wstring FilePath::fileName() const {
  return m_path->filename().generic_wstring();
}

std::wstring FilePath::extension() const {
  return m_path->extension().generic_wstring();
}

FilePath FilePath::withoutExtension() const {
  return FilePath(getPath().replace_extension().wstring());
}

FilePath FilePath::replaceExtension(const std::wstring& extension) const {
  return FilePath(getPath().replace_extension(extension).wstring());
}

bool FilePath::hasExtension(const std::vector<std::wstring>& extensions) const {
  const std::wstring e = extension();
  for(const std::wstring& ext : extensions) {
    if(e == ext) {
      return true;
    }
  }
  return false;
}

FilePath& FilePath::operator=(const FilePath& other) {
  m_path = std::make_unique<fs::path>(other.getPath());
  m_exists = other.m_exists;
  m_isDirectory = other.m_isDirectory;
  m_canonicalized = other.m_canonicalized;
  return *this;
}

FilePath& FilePath::operator=(FilePath&& other) noexcept {
  m_path = std::move(other.m_path);
  m_exists = other.m_exists;
  m_isDirectory = other.m_isDirectory;
  m_canonicalized = other.m_canonicalized;
  return *this;
}

bool FilePath::operator==(const FilePath& other) const {
  if(exists() && other.exists()) {
    std::error_code ec;
    bool equivalent = fs::equivalent(getPath(), other.getPath(), ec);
    if(ec) {
      LOG_ERROR(ec.message());
      return false;
    }
    return equivalent;
  }

  return m_path->compare(other.getPath()) == 0;
}

bool FilePath::operator!=(const FilePath& other) const {
  return !(*this == other);
}

bool FilePath::operator<(const FilePath& other) const {
  return m_path->compare(other.getPath()) < 0;
}