#include "SharedIndexerCommand.h"
#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "IndexerCommandCxx.h"
#else
#  include "IndexerCommand.h"
#endif

#include "logging.h"
#include "utilityString.h"

void SharedIndexerCommand::fromLocal(IndexerCommand* indexerCommand) {
  setSourceFilePath(indexerCommand->getSourceFilePath());

#if BUILD_CXX_LANGUAGE_PACKAGE
  if(dynamic_cast<IndexerCommandCxx*>(indexerCommand) != nullptr) {
    auto* cmd = dynamic_cast<IndexerCommandCxx*>(indexerCommand);

    setType(CXX);
    setIndexedPaths(cmd->getIndexedPaths());
    setExcludeFilters(cmd->getExcludeFilters());
    setIncludeFilters(cmd->getIncludeFilters());
    setWorkingDirectory(cmd->getWorkingDirectory());
    setCompilerFlags(cmd->getCompilerFlags());
    return;
  }
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

  LOG_ERROR(L"Trying to push unhandled type of IndexerCommand for file: " + indexerCommand->getSourceFilePath().wstr() +
            L". Type string is: " + utility::decodeFromUtf8(indexerCommandTypeToString(indexerCommand->getIndexerCommandType())) +
            L". It will be ignored.");
}

std::shared_ptr<IndexerCommand> SharedIndexerCommand::fromShared(const SharedIndexerCommand& indexerCommand) {
  switch(indexerCommand.getType()) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  case CXX:
    return std::make_shared<IndexerCommandCxx>(indexerCommand.getSourceFilePath(),
                                               indexerCommand.getIndexedPaths(),
                                               indexerCommand.getExcludeFilters(),
                                               indexerCommand.getIncludeFilters(),
                                               indexerCommand.getWorkingDirectory(),
                                               indexerCommand.getCompilerFlags());
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  case UNKNOWN:
  default:
    LOG_ERROR(L"Cannot convert shared IndexerCommand for file: " + indexerCommand.getSourceFilePath().wstr() +
              L". The type is unknown.");
  }

  return nullptr;
}

SharedIndexerCommand::SharedIndexerCommand(SharedMemory::Allocator* allocator)
    : m_type(Type::UNKNOWN)
    , m_sourceFilePath("", allocator)
#if BUILD_CXX_LANGUAGE_PACKAGE
    , m_indexedPaths(allocator)
    , m_excludeFilters(allocator)
    , m_includeFilters(allocator)
    , m_workingDirectory("", allocator)
    , m_compilerFlags(allocator)
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
{
}

SharedIndexerCommand::~SharedIndexerCommand() = default;

FilePath SharedIndexerCommand::getSourceFilePath() const {
  return FilePath(utility::decodeFromUtf8(m_sourceFilePath.c_str()));
}

void SharedIndexerCommand::setSourceFilePath(const FilePath& filePath) {
  m_sourceFilePath = utility::encodeToUtf8(filePath.wstr()).c_str();
}

#if BUILD_CXX_LANGUAGE_PACKAGE

std::set<FilePath> SharedIndexerCommand::getIndexedPaths() const {
  std::set<FilePath> result;

  for(const auto& m_indexedPath : m_indexedPaths) {
    result.insert(FilePath(utility::decodeFromUtf8(m_indexedPath.c_str())));
  }

  return result;
}

void SharedIndexerCommand::setIndexedPaths(const std::set<FilePath>& indexedPaths) {
  m_indexedPaths.clear();

  for(const FilePath& indexedPath : indexedPaths) {
    SharedMemory::String path(m_indexedPaths.get_allocator());
    path = utility::encodeToUtf8(indexedPath.wstr()).c_str();
    m_indexedPaths.push_back(path);
  }
}

std::set<FilePathFilter> SharedIndexerCommand::getExcludeFilters() const {
  std::set<FilePathFilter> result;

  for(const auto& m_excludeFilter : m_excludeFilters) {
    result.insert(FilePathFilter(utility::decodeFromUtf8(m_excludeFilter.c_str())));
  }

  return result;
}

void SharedIndexerCommand::setExcludeFilters(const std::set<FilePathFilter>& excludeFilters) {
  m_excludeFilters.clear();

  for(const FilePathFilter& excludeFilter : excludeFilters) {
    SharedMemory::String path(m_excludeFilters.get_allocator());
    path = utility::encodeToUtf8(excludeFilter.wstr()).c_str();
    m_excludeFilters.push_back(path);
  }
}

std::set<FilePathFilter> SharedIndexerCommand::getIncludeFilters() const {
  std::set<FilePathFilter> result;

  for(const auto& m_includeFilter : m_includeFilters) {
    result.insert(FilePathFilter(utility::decodeFromUtf8(m_includeFilter.c_str())));
  }

  return result;
}

void SharedIndexerCommand::setIncludeFilters(const std::set<FilePathFilter>& includeFilters) {
  m_includeFilters.clear();

  for(const FilePathFilter& includeFilter : includeFilters) {
    SharedMemory::String path(m_includeFilters.get_allocator());
    path = utility::encodeToUtf8(includeFilter.wstr()).c_str();
    m_includeFilters.push_back(path);
  }
}

FilePath SharedIndexerCommand::getWorkingDirectory() const {
  return FilePath(utility::decodeFromUtf8(m_workingDirectory.c_str()));
}

void SharedIndexerCommand::setWorkingDirectory(const FilePath& workingDirectory) {
  m_workingDirectory = utility::encodeToUtf8(workingDirectory.wstr()).c_str();
}

std::vector<std::wstring> SharedIndexerCommand::getCompilerFlags() const {
  std::vector<std::wstring> result;
  result.reserve(m_compilerFlags.size());

  for(const auto& m_compilerFlag : m_compilerFlags) {
    result.push_back(utility::decodeFromUtf8(m_compilerFlag.c_str()));
  }

  return result;
}

void SharedIndexerCommand::setCompilerFlags(const std::vector<std::wstring>& compilerFlags) {
  m_compilerFlags.clear();
  m_compilerFlags.reserve(compilerFlags.size());

  for(const std::wstring& compilerFlag : compilerFlags) {
    SharedMemory::String path(m_compilerFlags.get_allocator());
    path = utility::encodeToUtf8(compilerFlag).c_str();
    m_compilerFlags.push_back(path);
  }
}

#endif    // BUILD_CXX_LANGUAGE_PACKAGE

SharedIndexerCommand::Type SharedIndexerCommand::getType() const {
  return m_type;
}

void SharedIndexerCommand::setType(const SharedIndexerCommand::Type type) {
  m_type = type;
}
