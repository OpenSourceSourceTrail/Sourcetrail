#include "IndexerCommandJava.h"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>

#include "utilityString.h"

IndexerCommandType IndexerCommandJava::getStaticIndexerCommandType() {
  return INDEXER_COMMAND_JAVA;
}

IndexerCommandJava::IndexerCommandJava(const FilePath& sourceFilePath,
                                       std::set<FilePath> classPaths,
                                       std::wstring languageStandard)
    : IndexerCommand(sourceFilePath)
    , m_classPaths(std::move(classPaths))
    , m_languageStandard(std::move(languageStandard)) {}

IndexerCommandType IndexerCommandJava::getIndexerCommandType() const {
  return getStaticIndexerCommandType();
}

size_t IndexerCommandJava::getByteSize(size_t stringSize) const {
  size_t size = IndexerCommand::getByteSize(stringSize);

  for(const FilePath& classPath : m_classPaths) {
    size += stringSize + utility::encodeToUtf8(classPath.wstr()).size();
  }

  size += stringSize + utility::encodeToUtf8(m_languageStandard).size();

  return size;
}

const std::set<FilePath>& IndexerCommandJava::getClassPaths() const {
  return m_classPaths;
}

const std::wstring& IndexerCommandJava::getLanguageStandard() const {
  return m_languageStandard;
}

boost::json::object IndexerCommandJava::doSerialize() const {
  boost::json::object jsonObject = IndexerCommand::doSerialize();

  {
    boost::json::array classPathsArray;
    for(const FilePath& classPath : m_classPaths) {
      classPathsArray.emplace_back(utility::encodeToUtf8(classPath.wstr()));
    }
    jsonObject["class_paths"] = std::move(classPathsArray);
  }
  jsonObject["language_standard"] = utility::encodeToUtf8(m_languageStandard);

  return jsonObject;
}
