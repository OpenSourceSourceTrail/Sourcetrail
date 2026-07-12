#pragma once
#include <map>
#include <memory>
#include <set>

#include "Storage.h"

class IntermediateStorage : public Storage {
public:
  IntermediateStorage();

  ~IntermediateStorage() override;

  void clear();

  [[nodiscard]] size_t getByteSize(size_t stringSize) const;

  [[nodiscard]] size_t getSourceLocationCount() const {
    return mSourceLocations.size();
  }

  [[nodiscard]] bool hasFatalErrors() const;

  void setAllFilesIncomplete();

  void setFilesWithErrorsIncomplete();

  std::pair<Id, bool> addNode(const StorageNodeData& nodeData) override;

  std::vector<Id> addNodes(const std::vector<StorageNode>& nodes) override;

  void setNodeType(Id nodeId, int nodeType);

  void addSymbol(const StorageSymbol& symbol) override {
    mSymbols.push_back(symbol);
  }

  void addSymbols(const std::vector<StorageSymbol>& symbols) override {
    mSymbols.insert(mSymbols.end(), symbols.begin(), symbols.end());
  }

  void addFile(const StorageFile& file) override;

  void setFileLanguage(Id fileId, const std::wstring& languageIdentifier);

  Id addEdge(const StorageEdgeData& edgeData) override;

  std::vector<Id> addEdges(const std::vector<StorageEdge>& edges) override;

  Id addLocalSymbol(const StorageLocalSymbolData& localSymbolData) override;

  std::vector<Id> addLocalSymbols(const std::set<StorageLocalSymbol>& symbols) override;

  Id addSourceLocation(const StorageSourceLocationData& sourceLocationData) override;

  std::vector<Id> addSourceLocations(const std::vector<StorageSourceLocation>& locations) override;

  void addOccurrence(const StorageOccurrence& occurrence) override {
    mOccurrences.emplace(occurrence);
  }

  void addOccurrences(const std::vector<StorageOccurrence>& occurrences) override {
    mOccurrences.insert(occurrences.begin(), occurrences.end());
  }

  void addComponentAccess(const StorageComponentAccess& componentAccess) override {
    mComponentAccesses.emplace(componentAccess);
  }

  void addComponentAccesses(const std::vector<StorageComponentAccess>& componentAccesses) override {
    mComponentAccesses.insert(componentAccesses.begin(), componentAccesses.end());
  }

  void addElementComponent(const StorageElementComponent& component) override {
    mElementComponents.emplace(component);
  }

  void addElementComponents(const std::vector<StorageElementComponent>& components) override {
    mElementComponents.insert(components.begin(), components.end());
  }

  Id addError(const StorageErrorData& errorData) override;

  [[nodiscard]] const std::vector<StorageNode>& getStorageNodes() const override {
    return mNodes;
  }

  [[nodiscard]] const std::vector<StorageFile>& getStorageFiles() const override {
    return mFiles;
  }

  [[nodiscard]] const std::vector<StorageSymbol>& getStorageSymbols() const override {
    return mSymbols;
  }

  [[nodiscard]] const std::vector<StorageEdge>& getStorageEdges() const override {
    return mEdges;
  }

  [[nodiscard]] const std::set<StorageLocalSymbol>& getStorageLocalSymbols() const override {
    return mLocalSymbols;
  }

  [[nodiscard]] const std::set<StorageSourceLocation>& getStorageSourceLocations() const override {
    return mSourceLocations;
  }

  [[nodiscard]] const std::set<StorageOccurrence>& getStorageOccurrences() const override {
    return mOccurrences;
  }

  [[nodiscard]] const std::set<StorageComponentAccess>& getComponentAccesses() const override {
    return mComponentAccesses;
  }

  [[nodiscard]] const std::set<StorageElementComponent>& getElementComponents() const override {
    return mElementComponents;
  }

  [[nodiscard]] const std::vector<StorageError>& getErrors() const override {
    return mErrors;
  }

  void setStorageNodes(std::vector<StorageNode> storageNodes);

  void setStorageFiles(std::vector<StorageFile> storageFiles);

  void setStorageSymbols(std::vector<StorageSymbol> storageSymbols) {
    mSymbols = std::move(storageSymbols);
  }

  void setStorageEdges(std::vector<StorageEdge> storageEdges);

  void setStorageLocalSymbols(std::set<StorageLocalSymbol> storageLocalSymbols) {
    mLocalSymbols = std::move(storageLocalSymbols);
  }

  void setStorageSourceLocations(std::set<StorageSourceLocation> storageSourceLocations) {
    mSourceLocations = std::move(storageSourceLocations);
  }

  void setStorageOccurrences(std::set<StorageOccurrence> storageOccurrences) {
    mOccurrences = std::move(storageOccurrences);
  }

  void setComponentAccesses(std::set<StorageComponentAccess> componentAccesses) {
    mComponentAccesses = std::move(componentAccesses);
  }

  void setErrors(std::vector<StorageError> errors);

  [[nodiscard]] Id getNextId() const {
    return mNextId;
  }

  void setNextId(Id nextId) {
    mNextId = nextId;
  }

private:
  std::map<StorageNodeData, size_t> mNodesIndex;
  std::map<Id, size_t> mNodeIdIndex;
  std::vector<StorageNode> mNodes;

  std::map<StorageFile, size_t> mFilesIndex;    // this is used to prevent duplicates (unique)
  std::map<Id, size_t> mFilesIdIndex;
  std::vector<StorageFile> mFiles;

  std::vector<StorageSymbol> mSymbols;

  std::map<StorageEdgeData, size_t> mEdgesIndex;
  std::vector<StorageEdge> mEdges;

  std::set<StorageLocalSymbol> mLocalSymbols;

  std::set<StorageSourceLocation> mSourceLocations;

  std::set<StorageOccurrence> mOccurrences;

  std::set<StorageComponentAccess> mComponentAccesses;
  std::set<StorageElementComponent> mElementComponents;

  std::map<StorageErrorData, size_t> mErrorsIndex;    // this is used to prevent duplicates (unique)
  std::vector<StorageError> mErrors;

  Id mNextId = 1;
};