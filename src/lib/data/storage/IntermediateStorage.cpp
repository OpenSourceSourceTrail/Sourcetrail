#include "IntermediateStorage.h"

#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/transform.hpp>

#include "LocationType.h"
#include "utility.h"

IntermediateStorage::IntermediateStorage() = default;

IntermediateStorage::~IntermediateStorage() = default;

void IntermediateStorage::clear() {
  mNodesIndex.clear();
  mNodeIdIndex.clear();
  mNodes.clear();

  mFilesIndex.clear();
  mFilesIdIndex.clear();
  mFiles.clear();

  mSymbols.clear();

  mEdgesIndex.clear();
  mEdges.clear();

  mLocalSymbols.clear();
  mSourceLocations.clear();
  mOccurrences.clear();
  mComponentAccesses.clear();

  mErrorsIndex.clear();
  mErrors.clear();

  mNextId = 1;
}

size_t IntermediateStorage::getByteSize(size_t stringSize) const {
  size_t byteSize = 0;

  for(const StorageFile& storageFile : getStorageFiles()) {
    byteSize += sizeof(StorageFile);
    byteSize += stringSize + storageFile.filePath.size();
    byteSize += stringSize + storageFile.modificationTime.size();
  }

  for(const StorageErrorData& storageError : getErrors()) {
    byteSize += sizeof(StorageErrorData);
    byteSize += stringSize + storageError.message.size();
    byteSize += stringSize + storageError.translationUnit.size();
  }

  for(const StorageNode& storageNode : getStorageNodes()) {
    byteSize += sizeof(StorageNode);
    byteSize += stringSize + storageNode.serializedName.size();
  }

  for(const StorageLocalSymbol& storageLocalSymbol : getStorageLocalSymbols()) {
    byteSize += sizeof(StorageLocalSymbol);
    byteSize += stringSize + storageLocalSymbol.name.size();
  }

  byteSize += sizeof(StorageEdge) * getStorageEdges().size();
  byteSize += sizeof(StorageComponentAccess) * getComponentAccesses().size();
  byteSize += sizeof(StorageOccurrence) * getStorageOccurrences().size();
  byteSize += sizeof(StorageSymbol) * getStorageSymbols().size();
  byteSize += sizeof(StorageSourceLocation) * getStorageSourceLocations().size();

  return byteSize;
}

bool IntermediateStorage::hasFatalErrors() const {
  return ranges::any_of(mErrors, [](const StorageErrorData& error) { return error.fatal; });
}

void IntermediateStorage::setAllFilesIncomplete() {
  for(StorageFile& file : mFiles) {
    file.complete = false;
  }
}

void IntermediateStorage::setFilesWithErrorsIncomplete() {
  std::set<Id> errorFileIds;
  for(const StorageSourceLocation& location : mSourceLocations) {
    if(location.type == locationTypeToInt(LOCATION_ERROR)) {
      errorFileIds.insert(location.fileNodeId);
    }
  }

  for(StorageFile& file : mFiles) {
    if(errorFileIds.find(file.id) != errorFileIds.end()) {
      file.complete = false;
    }
  }
}

std::pair<Id, bool> IntermediateStorage::addNode(const StorageNodeData& nodeData) {
  if(auto found = mNodesIndex.find(nodeData); found != mNodesIndex.end()) {
    StorageNode& storedNode = mNodes[found->second];
    if(storedNode.type < nodeData.type) {
      storedNode.type = nodeData.type;
    }
    return {storedNode.id, false};
  }

  const Id nodeId = mNextId++;
  mNodes.emplace_back(nodeId, nodeData);
  mNodesIndex.emplace(nodeData, mNodes.size() - 1);
  mNodeIdIndex.emplace(nodeId, mNodes.size() - 1);
  return {nodeId, true};
}

std::vector<Id> IntermediateStorage::addNodes(const std::vector<StorageNode>& nodes) {
  return nodes | ranges::views::transform([this](const StorageNode& node) -> Id { return addNode(node).first; }) |
      ranges::to<std::vector>();
}

void IntermediateStorage::setNodeType(Id nodeId, int nodeType) {
  if(auto found = mNodeIdIndex.find(nodeId); found != mNodeIdIndex.end() && mNodes[found->second].type < nodeType) {
    mNodes[found->second].type = nodeType;
  }
}

void IntermediateStorage::addFile(const StorageFile& file) {
  if(auto found = mFilesIndex.find(file); found != mFilesIndex.end()) {
    StorageFile& storedFile = mFiles[found->second];

    if(file.indexed) {
      storedFile.indexed = true;
    }

    if(file.complete) {
      storedFile.complete = true;
    }

    if(!file.languageIdentifier.empty()) {
      storedFile.languageIdentifier = file.languageIdentifier;
    }
  } else {
    mFilesIndex.emplace(file, mFiles.size());
    mFilesIdIndex.emplace(file.id, mFiles.size());
    mFiles.emplace_back(file);
  }
}

void IntermediateStorage::setFileLanguage(Id fileId, const std::wstring& languageIdentifier) {
  if(auto found = mFilesIdIndex.find(fileId); found != mFilesIdIndex.end()) {
    mFiles[found->second].languageIdentifier = languageIdentifier;
  }
}

Id IntermediateStorage::addEdge(const StorageEdgeData& edgeData) {
  if(auto found = mEdgesIndex.find(edgeData); found != mEdgesIndex.end()) {
    return mEdges[found->second].id;
  }

  const Id edgeId = mNextId++;
  mEdges.emplace_back(edgeId, edgeData);
  mEdgesIndex.emplace(edgeData, mEdges.size() - 1);
  return edgeId;
}

std::vector<Id> IntermediateStorage::addEdges(const std::vector<StorageEdge>& edges) {
  return edges | ranges::views::transform([this](const StorageEdge& edge) -> Id { return addEdge(edge); }) |
      ranges::to<std::vector>();
}

Id IntermediateStorage::addLocalSymbol(const StorageLocalSymbolData& localSymbolData) {
  if(auto found = mLocalSymbols.find(StorageLocalSymbol(0, localSymbolData)); found != mLocalSymbols.end()) {
    return found->id;
  }

  const Id localSymbolId = mNextId++;
  mLocalSymbols.emplace(localSymbolId, localSymbolData);
  return localSymbolId;
}

std::vector<Id> IntermediateStorage::addLocalSymbols(const std::set<StorageLocalSymbol>& symbols) {
  return symbols | ranges::views::transform([this](const StorageLocalSymbol& symbol) -> Id { return addLocalSymbol(symbol); }) |
      ranges::to<std::vector>();
}

Id IntermediateStorage::addSourceLocation(const StorageSourceLocationData& sourceLocationData) {
  if(auto found = mSourceLocations.find(StorageSourceLocation(0, sourceLocationData)); found != mSourceLocations.end()) {
    return found->id;
  }

  const Id sourceLocationId = mNextId++;
  mSourceLocations.emplace(sourceLocationId, sourceLocationData);
  return sourceLocationId;
}

std::vector<Id> IntermediateStorage::addSourceLocations(const std::vector<StorageSourceLocation>& locations) {
  return locations |
      ranges::views::transform([this](const StorageSourceLocation& location) -> Id { return addSourceLocation(location); }) |
      ranges::to<std::vector>();
}

Id IntermediateStorage::addError(const StorageErrorData& errorData) {
  if(auto found = mErrorsIndex.find(errorData); found != mErrorsIndex.end()) {
    return mErrors[found->second].id;
  }

  const Id errorId = mNextId++;
  mErrors.emplace_back(errorId, errorData);
  mErrorsIndex.emplace(errorData, mErrors.size() - 1);
  return errorId;
}

void IntermediateStorage::setStorageNodes(std::vector<StorageNode> storageNodes) {
  mNodes = std::move(storageNodes);

  mNodesIndex.clear();
  mNodeIdIndex.clear();
  for(const auto& [index, node] : ranges::views::enumerate(mNodes)) {
    mNodesIndex.emplace(node, index);
    mNodeIdIndex.emplace(node.id, index);
  }
}

void IntermediateStorage::setStorageFiles(std::vector<StorageFile> storageFiles) {
  mFiles = std::move(storageFiles);

  mFilesIndex.clear();
  mFilesIdIndex.clear();
  for(const auto& [index, file] : ranges::views::enumerate(mFiles)) {
    mFilesIndex.emplace(file, index);
    mFilesIdIndex.emplace(file.id, index);
  }
}

void IntermediateStorage::setStorageEdges(std::vector<StorageEdge> storageEdges) {
  mEdges = std::move(storageEdges);

  mEdgesIndex = mEdges | ranges::views::enumerate |
      ranges::views::transform([](const auto& item) -> std::pair<StorageEdgeData, size_t> { return {item.second, item.first}; }) |
      ranges::to<std::map>();
}

void IntermediateStorage::setErrors(std::vector<StorageError> errors) {
  mErrors = std::move(errors);

  mErrorsIndex = mErrors | ranges::views::enumerate |
      ranges::views::transform([](const auto& item) -> std::pair<StorageErrorData, size_t> { return {item.second, item.first}; }) |
      ranges::to<std::map>();
}
