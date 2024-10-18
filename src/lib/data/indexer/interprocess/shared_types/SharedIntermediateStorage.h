#pragma once

#include <set>
#include <vector>

#include "SharedMemory.h"
#include "SharedStorageTypes.h"

class SharedIntermediateStorage {
public:
  SharedIntermediateStorage(SharedMemory::Allocator* allocator);
  ~SharedIntermediateStorage();

  std::vector<StorageNode> getStorageNodes() const;
  void setStorageNodes(const std::vector<StorageNode>& storageNodes);

  std::vector<StorageFile> getStorageFiles() const;
  void setStorageFiles(const std::vector<StorageFile>& storageFiles);

  std::vector<StorageSymbol> getStorageSymbols() const;
  void setStorageSymbols(const std::vector<StorageSymbol>& storageSymbols);

  std::vector<StorageEdge> getStorageEdges() const;
  void setStorageEdges(const std::vector<StorageEdge>& storageEdges);

  std::set<StorageLocalSymbol> getStorageLocalSymbols() const;
  void setStorageLocalSymbols(const std::set<StorageLocalSymbol>& storageLocalSymbols);

  std::set<StorageSourceLocation> getStorageSourceLocations() const;
  void setStorageSourceLocations(const std::set<StorageSourceLocation>& storageSourceLocations);

  std::set<StorageOccurrence> getStorageOccurrences() const;
  void setStorageOccurrences(const std::set<StorageOccurrence>& storageOccurrences);

  std::set<StorageComponentAccess> getStorageComponentAccesses() const;
  void setStorageComponentAccesses(const std::set<StorageComponentAccess>& storageComponentAccesses);

  std::vector<StorageError> getStorageErrors() const;
  void setStorageErrors(const std::vector<StorageError>& errors);

  Id getNextId() const;
  void setNextId(const Id nextId);

private:
  SharedMemory::Vector<SharedStorageFile> mStorageFiles;
  SharedMemory::Vector<SharedStorageSymbol> mStorageSymbols;
  SharedMemory::Vector<SharedStorageOccurrence> mStorageOccurrences;
  SharedMemory::Vector<SharedStorageComponentAccess> mStorageComponentAccesses;
  SharedMemory::Vector<SharedStorageNode> mStorageNodes;
  SharedMemory::Vector<SharedStorageEdge> mStorageEdges;
  SharedMemory::Vector<SharedStorageLocalSymbol> mStorageLocalSymbols;
  SharedMemory::Vector<SharedStorageSourceLocation> mStorageSourceLocations;
  SharedMemory::Vector<SharedStorageError> mStorageErrors;

  SharedMemory::Allocator* mAllocator;

  int mNextId;
};