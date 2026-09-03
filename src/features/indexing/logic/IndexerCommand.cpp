#include "indexing/logic/IndexerCommand.h"

#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>

#include "utilityString.h"

std::wstring IndexerCommand::serialize(std::shared_ptr<const IndexerCommand> indexerCommand, bool /*compact*/) {
  return utility::decodeFromUtf8(boost::json::serialize(indexerCommand->doSerialize()));
}

IndexerCommand::IndexerCommand(const FilePath& sourceFilePath) : m_sourceFilePath(sourceFilePath) {}

size_t IndexerCommand::getByteSize(size_t /*stringSize*/) const {
  return utility::encodeToUtf8(m_sourceFilePath.wstr()).size();
}

const FilePath& IndexerCommand::getSourceFilePath() const {
  return m_sourceFilePath;
}

boost::json::object IndexerCommand::doSerialize() const {
  boost::json::object jsonObject;

  jsonObject["type"] = indexerCommandTypeToString(getIndexerCommandType());
  jsonObject["source_file_path"] = utility::encodeToUtf8(m_sourceFilePath.wstr());

  return jsonObject;
}
