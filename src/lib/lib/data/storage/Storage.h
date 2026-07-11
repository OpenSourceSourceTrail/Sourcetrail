#pragma once
/**
 * @file Storage.h
 * @brief Defines the Storage class for managing various storage components.
 */
#include <functional>
#include <mutex>
#include <set>
#include <string>

#include "GlobalId.hpp"
#include "StorageComponentAccess.h"
#include "StorageEdge.h"
#include "StorageElementComponent.h"
#include "StorageError.h"
#include "StorageFile.h"
#include "StorageLocalSymbol.h"
#include "StorageNode.h"
#include "StorageOccurrence.h"
#include "StorageSourceLocation.h"
#include "StorageSymbol.h"

/**
 * @class Storage
 * @brief Abstract base class for managing storage of various components in a software system.
 *
 * This class provides interfaces for adding and retrieving different types of storage elements
 * such as nodes, symbols, files, edges, local symbols, source locations, occurrences,
 * component accesses, element components, and errors.
 */
class Storage {
public:
  /**
   * @brief Default constructor.
   */
  Storage();

  /**
   * @brief Virtual destructor.
   */
  virtual ~Storage();

  /**
   * @brief Adds a node to the storage.
   * @param data The StorageNodeData to be added.
   * @return A pair containing the ID of the added node and a boolean indicating if it's a new node.
   */
  virtual std::pair<Id, bool> addNode(const StorageNodeData& data) = 0;

  /**
   * @brief Adds multiple nodes to the storage.
   * @param nodes A vector of StorageNode objects to be added.
   * @return A vector of IDs for the added nodes.
   */
  virtual std::vector<Id> addNodes(const std::vector<StorageNode>& nodes) = 0;

  /**
   * @brief Adds a symbol to the storage.
   * @param data The StorageSymbol to be added.
   */
  virtual void addSymbol(const StorageSymbol& data) = 0;

  /**
   * @brief Adds multiple symbols to the storage.
   * @param symbols A vector of StorageSymbol objects to be added.
   */
  virtual void addSymbols(const std::vector<StorageSymbol>& symbols) = 0;

  /**
   * @brief Adds a file to the storage.
   * @param data The StorageFile to be added.
   */
  virtual void addFile(const StorageFile& data) = 0;

  /**
   * @brief Adds an edge to the storage.
   * @param data The StorageEdgeData to be added.
   * @return The ID of the added edge.
   */
  virtual Id addEdge(const StorageEdgeData& data) = 0;

  /**
   * @brief Adds multiple edges to the storage.
   * @param edges A vector of StorageEdge objects to be added.
   * @return A vector of IDs for the added edges.
   */
  virtual std::vector<Id> addEdges(const std::vector<StorageEdge>& edges) = 0;

  /**
   * @brief Adds a local symbol to the storage.
   * @param data The StorageLocalSymbolData to be added.
   * @return The ID of the added local symbol.
   */
  virtual Id addLocalSymbol(const StorageLocalSymbolData& data) = 0;

  /**
   * @brief Adds multiple local symbols to the storage.
   * @param symbols A set of StorageLocalSymbol objects to be added.
   * @return A vector of IDs for the added local symbols.
   */
  virtual std::vector<Id> addLocalSymbols(const std::set<StorageLocalSymbol>& symbols) = 0;

  /**
   * @brief Adds a source location to the storage.
   * @param data The StorageSourceLocationData to be added.
   * @return The ID of the added source location.
   */
  virtual Id addSourceLocation(const StorageSourceLocationData& data) = 0;

  /**
   * @brief Adds multiple source locations to the storage.
   * @param locations A vector of StorageSourceLocation objects to be added.
   * @return A vector of IDs for the added source locations.
   */
  virtual std::vector<Id> addSourceLocations(const std::vector<StorageSourceLocation>& locations) = 0;

  /**
   * @brief Adds an occurrence to the storage.
   * @param data The StorageOccurrence to be added.
   */
  virtual void addOccurrence(const StorageOccurrence& data) = 0;

  /**
   * @brief Adds multiple occurrences to the storage.
   * @param occurrences A vector of StorageOccurrence objects to be added.
   */
  virtual void addOccurrences(const std::vector<StorageOccurrence>& occurrences) = 0;

  /**
   * @brief Adds a component access to the storage.
   * @param componentAccess The StorageComponentAccess to be added.
   */
  virtual void addComponentAccess(const StorageComponentAccess& componentAccess) = 0;

  /**
   * @brief Adds multiple component accesses to the storage.
   * @param componentAccesses A vector of StorageComponentAccess objects to be added.
   */
  virtual void addComponentAccesses(const std::vector<StorageComponentAccess>& componentAccesses) = 0;

  /**
   * @brief Adds an element component to the storage.
   * @param component The StorageElementComponent to be added.
   */
  virtual void addElementComponent(const StorageElementComponent& component) = 0;

  /**
   * @brief Adds multiple element components to the storage.
   * @param components A vector of StorageElementComponent objects to be added.
   */
  virtual void addElementComponents(const std::vector<StorageElementComponent>& components) = 0;

  /**
   * @brief Adds an error to the storage.
   * @param data The StorageErrorData to be added.
   * @return The ID of the added error.
   */
  virtual Id addError(const StorageErrorData& data) = 0;

  /**
   * @brief Retrieves all storage nodes.
   * @return A const reference to the vector of StorageNode objects.
   */
  virtual const std::vector<StorageNode>& getStorageNodes() const = 0;

  /**
   * @brief Retrieves all storage files.
   * @return A const reference to the vector of StorageFile objects.
   */
  virtual const std::vector<StorageFile>& getStorageFiles() const = 0;

  /**
   * @brief Retrieves all storage symbols.
   * @return A const reference to the vector of StorageSymbol objects.
   */
  virtual const std::vector<StorageSymbol>& getStorageSymbols() const = 0;

  /**
   * @brief Retrieves all storage edges.
   * @return A const reference to the vector of StorageEdge objects.
   */
  virtual const std::vector<StorageEdge>& getStorageEdges() const = 0;

  /**
   * @brief Retrieves all storage local symbols.
   * @return A const reference to the set of StorageLocalSymbol objects.
   */
  virtual const std::set<StorageLocalSymbol>& getStorageLocalSymbols() const = 0;

  /**
   * @brief Retrieves all storage source locations.
   * @return A const reference to the set of StorageSourceLocation objects.
   */
  virtual const std::set<StorageSourceLocation>& getStorageSourceLocations() const = 0;

  /**
   * @brief Retrieves all storage occurrences.
   * @return A const reference to the set of StorageOccurrence objects.
   */
  virtual const std::set<StorageOccurrence>& getStorageOccurrences() const = 0;

  /**
   * @brief Retrieves all component accesses.
   * @return A const reference to the set of StorageComponentAccess objects.
   */
  virtual const std::set<StorageComponentAccess>& getComponentAccesses() const = 0;

  /**
   * @brief Retrieves all element components.
   * @return A const reference to the set of StorageElementComponent objects.
   */
  virtual const std::set<StorageElementComponent>& getElementComponents() const = 0;

  /**
   * @brief Retrieves all errors.
   * @return A const reference to the vector of StorageError objects.
   */
  virtual const std::vector<StorageError>& getErrors() const = 0;

  /**
   * @brief Injects another Storage object into this one.
   * @param injected Pointer to the Storage object to be injected.
   */
  void inject(Storage* injected);

private:
  /**
   * @brief Starts the injection process.
   */
  virtual void startInjection();

  /**
   * @brief Finishes the injection process.
   */
  virtual void finishInjection();

  std::mutex mDataMutex;    ///< Mutex for thread-safe data access
};