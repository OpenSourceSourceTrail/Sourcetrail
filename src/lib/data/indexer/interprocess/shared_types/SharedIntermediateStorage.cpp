#include "SharedIntermediateStorage.h"

SharedIntermediateStorage::SharedIntermediateStorage(SharedMemory::Allocator* allocator)
    : mStorageFiles(allocator)
    , mStorageSymbols(allocator)
    , mStorageOccurrences(allocator)
    , mStorageComponentAccesses(allocator)
    , mStorageNodes(allocator)
    , mStorageEdges(allocator)
    , mStorageLocalSymbols(allocator)
    , mStorageSourceLocations(allocator)
    , mStorageErrors(allocator)
    , mAllocator(allocator)
    , mNextId(1) {}

SharedIntermediateStorage::~SharedIntermediateStorage() = default;

std::vector<StorageFile> SharedIntermediateStorage::getStorageFiles() const {
  std::vector<StorageFile> result;
  result.reserve(mStorageFiles.size());

  for(const auto& mStorageFile : mStorageFiles) {
    result.emplace_back(fromShared(mStorageFile));
  }

  return result;
}

void SharedIntermediateStorage::setStorageFiles(const std::vector<StorageFile>& storageFiles) {
  mStorageFiles.clear();

  for(const auto& storageFile : storageFiles) {
    mStorageFiles.push_back(toShared(storageFile, mAllocator));
  }
}

std::vector<StorageNode> SharedIntermediateStorage::getStorageNodes() const {
  std::vector<StorageNode> result;
  result.reserve(mStorageNodes.size());

  for(const auto& mStorageNode : mStorageNodes) {
    result.emplace_back(fromShared(mStorageNode));
  }

  return result;
}

void SharedIntermediateStorage::setStorageNodes(const std::vector<StorageNode>& storageNodes) {
  mStorageNodes.clear();

  for(const auto& storageNode : storageNodes) {
    mStorageNodes.push_back(toShared(storageNode, mAllocator));
  }
}

std::vector<StorageSymbol> SharedIntermediateStorage::getStorageSymbols() const {
  std::vector<StorageSymbol> result;
  result.reserve(mStorageSymbols.size());

  for(auto mStorageSymbol : mStorageSymbols) {
    result.emplace_back(fromShared(mStorageSymbol));
  }

  return result;
}

void SharedIntermediateStorage::setStorageSymbols(const std::vector<StorageSymbol>& storageSymbols) {
  mStorageSymbols.clear();

  for(auto storageSymbol : storageSymbols) {
    mStorageSymbols.push_back(toShared(storageSymbol, mAllocator));
  }
}

std::vector<StorageEdge> SharedIntermediateStorage::getStorageEdges() const {
  std::vector<StorageEdge> result;
  result.reserve(mStorageEdges.size());

  for(const auto& mStorageEdge : mStorageEdges) {
    result.emplace_back(fromShared(mStorageEdge));
  }

  return result;
}

void SharedIntermediateStorage::setStorageEdges(const std::vector<StorageEdge>& storageEdges) {
  mStorageEdges.clear();

  for(const auto& storageEdge : storageEdges) {
    mStorageEdges.push_back(toShared(storageEdge, mAllocator));
  }
}

std::set<StorageLocalSymbol> SharedIntermediateStorage::getStorageLocalSymbols() const {
  std::set<StorageLocalSymbol> result;

  for(const auto& mStorageLocalSymbol : mStorageLocalSymbols) {
    result.emplace(fromShared(mStorageLocalSymbol));
  }

  return result;
}

void SharedIntermediateStorage::setStorageLocalSymbols(const std::set<StorageLocalSymbol>& storageLocalSymbols) {
  mStorageLocalSymbols.clear();

  for(const StorageLocalSymbol& localSymbol : storageLocalSymbols) {
    mStorageLocalSymbols.push_back(toShared(localSymbol, mAllocator));
  }
}

std::set<StorageSourceLocation> SharedIntermediateStorage::getStorageSourceLocations() const {
  std::set<StorageSourceLocation> result;

  for(const auto& mStorageSourceLocation : mStorageSourceLocations) {
    result.emplace(fromShared(mStorageSourceLocation));
  }

  return result;
}

void SharedIntermediateStorage::setStorageSourceLocations(const std::set<StorageSourceLocation>& storageSourceLocations) {
  mStorageSourceLocations.clear();

  for(const StorageSourceLocation& sourceLocation : storageSourceLocations) {
    mStorageSourceLocations.push_back(toShared(sourceLocation, mAllocator));
  }
}

std::set<StorageOccurrence> SharedIntermediateStorage::getStorageOccurrences() const {
  std::set<StorageOccurrence> result;

  for(auto mStorageOccurrence : mStorageOccurrences) {
    result.emplace(fromShared(mStorageOccurrence));
  }

  return result;
}

void SharedIntermediateStorage::setStorageOccurrences(const std::set<StorageOccurrence>& storageOccurrences) {
  mStorageOccurrences.clear();

  for(const StorageOccurrence& occurrence : storageOccurrences) {
    mStorageOccurrences.push_back(toShared(occurrence, mAllocator));
  }
}

std::set<StorageComponentAccess> SharedIntermediateStorage::getStorageComponentAccesses() const {
  std::set<StorageComponentAccess> result;

  for(auto mStorageComponentAccesse : mStorageComponentAccesses) {
    result.emplace(fromShared(mStorageComponentAccesse));
  }

  return result;
}

void SharedIntermediateStorage::setStorageComponentAccesses(const std::set<StorageComponentAccess>& storageComponentAccesses) {
  mStorageComponentAccesses.clear();

  for(const StorageComponentAccess& componentAccess : storageComponentAccesses) {
    mStorageComponentAccesses.push_back(toShared(componentAccess, mAllocator));
  }
}

std::vector<StorageError> SharedIntermediateStorage::getStorageErrors() const {
  std::vector<StorageError> result;
  result.reserve(mStorageErrors.size());

  for(const auto& mStorageError : mStorageErrors) {
    result.emplace_back(fromShared(mStorageError));
  }

  return result;
}

void SharedIntermediateStorage::setStorageErrors(const std::vector<StorageError>& errors) {
  mStorageErrors.clear();

  for(const auto& error : errors) {
    mStorageErrors.push_back(toShared(error, mAllocator));
  }
}

Id SharedIntermediateStorage::getNextId() const {
  return static_cast<size_t>(mNextId);
}

void SharedIntermediateStorage::setNextId(const Id nextId) {
  mNextId = static_cast<int>(nextId);
}
