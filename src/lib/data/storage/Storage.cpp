#include "Storage.h"

#include <cstddef>
#include <map>
#include <mutex>
#include <set>
#include <vector>

#include "logging.h"

Storage::Storage() = default;

Storage::~Storage() = default;

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Storage::inject(Storage* injected) {
  const std::lock_guard<std::mutex> lock(mDataMutex);

  std::map<Id, Id> injectedIdToOwnElementId;
  std::map<Id, Id> injectedIdToOwnSourceLocationId;

  startInjection();

  {
    for(const StorageError& error : injected->getErrors()) {
      Id errorId = addError(error);
      injectedIdToOwnElementId.emplace(error.id, errorId);
    }
  }

  {
    const std::vector<StorageNode>& nodes = injected->getStorageNodes();

    std::vector<Id> nodeIds = addNodes(nodes);

    for(size_t i = 0; i < nodes.size(); i++) {
      if(nodeIds[i] != 0U) {
        injectedIdToOwnElementId.emplace(nodes[i].id, nodeIds[i]);
      }
    }
  }

  {
    for(const StorageFile& file : injected->getStorageFiles()) {
      if(auto iterator = injectedIdToOwnElementId.find(file.id); iterator != injectedIdToOwnElementId.end()) {
        addFile(StorageFile(
            iterator->second, file.filePath, file.languageIdentifier, file.modificationTime, file.indexed, file.complete));
      }
    }
  }

  {
    std::vector<StorageSymbol> symbols = injected->getStorageSymbols();
    for(size_t i = 0; i < symbols.size(); i++) {
      if(auto iterator = injectedIdToOwnElementId.find(symbols[i].id); iterator != injectedIdToOwnElementId.end()) {
        symbols[i].id = iterator->second;
      } else {
        LOG_WARNING("New symbol id could not be found.");
        symbols.erase(symbols.begin() + static_cast<long>(i));
        i--;
      }
    }

    addSymbols(symbols);
  }

  {
    std::vector<StorageEdge> edges = injected->getStorageEdges();
    for(size_t i = 0; i < edges.size(); i++) {
      StorageEdge& edge = edges[i];
      size_t updateCount = 0;

      auto iterator = injectedIdToOwnElementId.find(edge.sourceNodeId);
      if(iterator != injectedIdToOwnElementId.end()) {
        edge.sourceNodeId = iterator->second;
        updateCount++;
      }

      iterator = injectedIdToOwnElementId.find(edge.targetNodeId);
      if(iterator != injectedIdToOwnElementId.end()) {
        edge.targetNodeId = iterator->second;
        updateCount++;
      }

      if(updateCount != 2) {
        LOG_WARNING("New edge source or target id could not be found.");
        edges.erase(edges.begin() + static_cast<long>(i));
        i--;
      }
    }

    std::vector<Id> edgeIds = addEdges(edges);

    if(edges.size() == edgeIds.size()) {
      for(size_t i = 0; i < edgeIds.size(); i++) {
        if(edgeIds[i] != 0U) {
          injectedIdToOwnElementId.emplace(edges[i].id, edgeIds[i]);
        }
      }
    } else {
      LOG_ERROR("Returned edge ids don't match injected count.");
    }
  }

  {
    const std::set<StorageLocalSymbol>& symbols = injected->getStorageLocalSymbols();
    std::vector<Id> symbolIds = addLocalSymbols(symbols);

    auto iterator = symbols.begin();
    for(size_t i = 0; i < symbols.size(); i++) {
      if(symbolIds[i] != 0U) {
        injectedIdToOwnElementId.emplace(iterator->id, symbolIds[i]);
      }
      iterator++;
    }
  }

  {
    const std::set<StorageSourceLocation>& oldLocations = injected->getStorageSourceLocations();
    std::vector<StorageSourceLocation> locations;
    locations.reserve(oldLocations.size());

    for(const StorageSourceLocation& location : oldLocations) {
      if(auto iterator = injectedIdToOwnElementId.find(location.fileNodeId); iterator != injectedIdToOwnElementId.end()) {
        const Id ownFileNodeId = iterator->second;
        locations.emplace_back(
            location.id, ownFileNodeId, location.startLine, location.startCol, location.endLine, location.endCol, location.type);
      }
    }

    std::vector<Id> locationIds = addSourceLocations(locations);

    if(locations.size() == locationIds.size()) {
      for(size_t i = 0; i < locationIds.size(); i++) {
        if(locationIds[i] != 0U) {
          injectedIdToOwnSourceLocationId.emplace(locations[i].id, locationIds[i]);
        }
      }
    } else {
      LOG_ERROR("Returned source locations ids don't match injected count.");
    }
  }

  {
    const std::set<StorageOccurrence>& oldOccurrences = injected->getStorageOccurrences();

    std::vector<StorageOccurrence> occurrences;
    occurrences.reserve(oldOccurrences.size());

    for(const StorageOccurrence& occurrence : oldOccurrences) {
      Id elementId = 0;
      Id sourceLocationId = 0;

      auto iterator = injectedIdToOwnElementId.find(occurrence.elementId);
      if(iterator != injectedIdToOwnElementId.end()) {
        elementId = iterator->second;
      }

      iterator = injectedIdToOwnSourceLocationId.find(occurrence.sourceLocationId);
      if(iterator != injectedIdToOwnSourceLocationId.end()) {
        sourceLocationId = iterator->second;
      }

      if(elementId == 0U) {
        LOG_WARNING("New occurrence element id could not be found.");
      } else if(sourceLocationId == 0U) {
        LOG_WARNING("New occurrence location id could not be found.");
      } else {
        occurrences.emplace_back(elementId, sourceLocationId);
      }
    }

    addOccurrences(occurrences);
  }

  {
    const std::set<StorageElementComponent>& oldComponents = injected->getElementComponents();
    std::vector<StorageElementComponent> components;
    components.reserve(oldComponents.size());

    for(const StorageElementComponent& component : oldComponents) {
      if(auto iterator = injectedIdToOwnElementId.find(component.elementId); iterator != injectedIdToOwnElementId.end()) {
        components.emplace_back(iterator->second, component.type, component.data);
      }
    }

    addElementComponents(components);
  }

  {
    const std::set<StorageComponentAccess>& oldAccesses = injected->getComponentAccesses();
    std::vector<StorageComponentAccess> accesses;
    accesses.reserve(oldAccesses.size());

    for(const StorageComponentAccess& access : oldAccesses) {
      if(auto iterator = injectedIdToOwnElementId.find(access.nodeId); iterator != injectedIdToOwnElementId.end()) {
        accesses.emplace_back(iterator->second, access.type);
      }
    }

    addComponentAccesses(accesses);
  }

  finishInjection();
}

void Storage::startInjection() {
  // may be implemented in derived
}

void Storage::finishInjection() {
  // may be implemented in derived
}
