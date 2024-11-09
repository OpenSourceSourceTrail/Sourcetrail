#pragma once
/**
 * @file SqliteIndexStorage.h
 * @brief SQLite-based storage implementation for indexing source code elements
 *
 * This class provides a SQLite-based storage system for managing and querying indexed source code elements including:
 * - Nodes (representing code entities)
 * - Edges (representing relationships between entities)
 * - Symbols and Local Symbols
 * - Source Locations
 * - Files and File Contents
 * - Occurrences
 * - Component Access information
 * - Error information
 *
 * The storage supports both read and write operations, with methods for:
 * - Adding/removing elements
 * - Querying elements by various criteria
 * - Managing file indexing status
 * - Batch operations for better performance
 *
 * @note This class inherits from SqliteStorage and implements the final storage layer
 * for the source code indexing system.
 */

#include <memory>
#include <string>
#include <vector>

#include "ErrorInfo.h"
#include "GlobalId.hpp"
#include "LocationType.h"
#include "LowMemoryStringMap.h"
#include "SqliteDatabaseIndex.h"
#include "SqliteStorage.h"
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
#include "utility.h"
#include "utilityString.h"

class TextAccess;
class Version;
class SourceLocationCollection;
class SourceLocationFile;

/**
 * @class SqliteIndexStorage
 * @brief SQLite-based storage implementation for managing indexed source code elements
 *
 * This class provides a comprehensive storage solution for source code indexing data using SQLite.
 * It manages various types of data including nodes, edges, symbols, files, and their relationships.
 *
 * Key features:
 * - CRUD operations for all source code elements
 * - Batch processing capabilities for better performance
 * - File content and indexing status management
 * - Query operations for retrieving elements by various criteria
 * - Error tracking and management
 *
 * Storage modes:
 * - STORAGE_MODE_READ: Read-only access to the database
 * - STORAGE_MODE_WRITE: Read-write access to the database
 * - STORAGE_MODE_CLEAR: Allows clearing of database contents
 *
 * @note This class is marked as final and inherits from SqliteStorage
 *
 * Example usage:
 * @code
 * SqliteIndexStorage storage(FilePath{"path/to/database.db"});
 * storage.setMode(SqliteIndexStorage::STORAGE_MODE_WRITE);
 *
 * // Add a node
 * StorageNodeData nodeData;
 * Id nodeId = storage.addNode(nodeData);
 *
 * // Query nodes
 * StorageNode node = storage.getNodeById(nodeId);
 * @endcode
 */
class SqliteIndexStorage final : public SqliteStorage {
public:
  /**
   * @brief Returns the storage version
   * @return The storage version
   */
  static size_t getStorageVersion();

  /**
   * @brief Storage modes for the storage
   */
  enum StorageModeType { STORAGE_MODE_READ = 1, STORAGE_MODE_WRITE = 2, STORAGE_MODE_CLEAR = 4 };

  /**
   * @brief Default constructor that initializes an in-memory SQLite database
   */
  SqliteIndexStorage();

  /**
   * @brief Constructor that initializes a SQLite database at the specified file path
   * @param dbFilePath Path to the SQLite database file
   */
  explicit SqliteIndexStorage(const FilePath& dbFilePath);

  /**
   * @brief Returns the static version of the storage
   * @return Static version of the storage
   */
  size_t getStaticVersion() const override;

  /**
   * @brief Sets the mode of the storage
   * @param mode The mode to set
   */
  void setMode(StorageModeType mode);

  /**
   * @brief Returns the project settings text
   * @return Project settings text
   */
  std::string getProjectSettingsText() const;

  /**
   * @brief Sets the project settings text
   * @param text The text to set
   */
  void setProjectSettingsText(std::string text);

  /**
   * @brief Adds a node to the storage
   * @param data The data to add
   * @return The ID of the added node
   */
  Id addNode(const StorageNodeData& data);

  /**
   * @brief Adds multiple nodes to the storage
   * @param nodes The nodes to add
   * @return The IDs of the added nodes
   */
  std::vector<Id> addNodes(const std::vector<StorageNode>& nodes);

  /**
   * @brief Adds a symbol to the storage
   * @param data The data to add
   * @return True if the symbol was added successfully, false otherwise
   */
  bool addSymbol(const StorageSymbol& data);

  /**
   * @brief Adds multiple symbols to the storage
   * @param symbols The symbols to add
   * @return True if all symbols were added successfully, false otherwise
   */
  bool addSymbols(const std::vector<StorageSymbol>& symbols);

  /**
   * @brief Adds multiple symbols to the storage
   * @brief Adds a file to the storage
   * @param data The data to add
   * @return True if the file was added successfully, false otherwise
   */
  bool addFile(const StorageFile& data);

  /**
   * @brief Adds an edge to the storage
   * @param data The data to add
   * @return The ID of the added edge
   */
  Id addEdge(const StorageEdgeData& data);

  /**
   * @brief Adds multiple edges to the storage
   * @param edges The edges to add
   * @return The IDs of the added edges
   */
  std::vector<Id> addEdges(const std::vector<StorageEdge>& edges);

  /**
   * @brief Adds a local symbol to the storage
   * @param data The data to add
   * @return The ID of the added local symbol
   */
  Id addLocalSymbol(const StorageLocalSymbolData& data);

  /**
   * @brief Adds multiple local symbols to the storage
   * @param symbols The symbols to add
   * @return The IDs of the added local symbols
   */
  std::vector<Id> addLocalSymbols(const std::set<StorageLocalSymbol>& symbols);

  /**
   * @brief Adds a source location to the storage
   * @param data The data to add
   * @return The ID of the added source location
   */
  Id addSourceLocation(const StorageSourceLocationData& data);

  /**
   * @brief Adds multiple source locations to the storage
   * @param locations The locations to add
   * @return The IDs of the added source locations
   */
  std::vector<Id> addSourceLocations(const std::vector<StorageSourceLocation>& locations);

  /**
   * @brief Adds an occurrence to the storage
   * @param data The data to add
   * @return True if the occurrence was added successfully, false otherwise
   */
  bool addOccurrence(const StorageOccurrence& data);

  /**
   * @brief Adds multiple occurrences to the storage
   * @param occurrences The occurrences to add
   * @return True if all occurrences were added successfully, false otherwise
   */
  bool addOccurrences(const std::vector<StorageOccurrence>& occurrences);

  /**
   * @brief Adds a component access to the storage
   * @param componentAccess The component access to add
   * @return True if the component access was added successfully, false otherwise
   */
  bool addComponentAccess(const StorageComponentAccess& componentAccess);

  /**
   * @brief Adds multiple component accesses to the storage
   * @param componentAccesses The component accesses to add
   * @return True if all component accesses were added successfully, false otherwise
   */
  bool addComponentAccesses(const std::vector<StorageComponentAccess>& componentAccesses);

  /**
   * @brief Adds an element component to the storage
   * @param component The component to add
   */
  void addElementComponent(const StorageElementComponent& component);

  /**
   * @brief Adds multiple element components to the storage
   * @param components The components to add
   */
  void addElementComponents(const std::vector<StorageElementComponent>& components);

  /**
   * @brief Adds an error to the storage
   * @param data The data to add
   * @return The ID of the added error
   */
  StorageError addError(const StorageErrorData& data);

  /**
   * @brief Removes an element from the storage
   * @param id The ID of the element to remove
   */
  void removeElement(Id id);

  /**
   * @brief Removes multiple elements from the storage
   * @param ids The IDs of the elements to remove
   */
  void removeElements(const std::vector<Id>& ids);

  /**
   * @brief Removes an occurrence from the storage
   * @param occurrence The occurrence to remove
   */
  void removeOccurrence(const StorageOccurrence& occurrence);

  /**
   * @brief Removes multiple occurrences from the storage
   * @param occurrences The occurrences to remove
   */
  void removeOccurrences(const std::vector<StorageOccurrence>& occurrences);

  /**
   * @brief Removes elements without occurrences from the storage
   * @param elementIds The IDs of the elements to remove
   */
  void removeElementsWithoutOccurrences(const std::vector<Id>& elementIds);

  /**
   * @brief Removes elements with locations in specified files from the storage
   * @param fileIds The IDs of the files to remove
   * @param updateStatusCallback The callback to update the status
   */
  void removeElementsWithLocationInFiles(const std::vector<Id>& fileIds, std::function<void(int)> updateStatusCallback);

  /**
   * @brief Removes all errors from the storage
   */
  void removeAllErrors();

  /**
   * @brief Checks if an element is an edge
   * @param elementId The ID of the element to check
   * @return True if the element is an edge, false otherwise
   */
  bool isEdge(Id elementId) const;

  /**
   * @brief Checks if an element is a node
   * @param elementId The ID of the element to check
   * @return True if the element is a node, false otherwise
   */
  bool isNode(Id elementId) const;

  /**
   * @brief Checks if an element is a file
   * @param elementId The ID of the element to check
   * @return True if the element is a file, false otherwise
   */
  bool isFile(Id elementId) const;

  /**
   * @brief Returns an edge by its ID
   * @param edgeId The ID of the edge to return
   * @return The edge
   */
  StorageEdge getEdgeById(Id edgeId) const;

  /**
   * @brief Returns an edge by its source, target, and type
   * @param sourceId The ID of the source node
   * @param targetId The ID of the target node
   * @param type The type of the edge
   * @return The edge
   */
  StorageEdge getEdgeBySourceTargetType(Id sourceId, Id targetId, int type) const;

  /**
   * @brief Returns edges by their source ID
   * @param sourceId The ID of the source node
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesBySourceId(Id sourceId) const;

  /**
   * @brief Returns edges by their source IDs
   * @param sourceIds The IDs of the source nodes
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesBySourceIds(const std::vector<Id>& sourceIds) const;

  /**
   * @brief Returns edges by their target ID
   * @param targetId The ID of the target node
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesByTargetId(Id targetId) const;

  /**
   * @brief Returns edges by their target IDs
   * @param targetIds The IDs of the target nodes
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesByTargetIds(const std::vector<Id>& targetIds) const;

  /**
   * @brief Returns edges by their source or target ID
   * @param id The ID of the source or target node
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesBySourceOrTargetId(Id id) const;

  /**
   * @brief Returns edges by their type
   * @param type The type of the edges
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesByType(int type) const;

  /**
   * @brief Returns edges by their source ID and type
   * @param sourceId The ID of the source node
   * @param type The type of the edges
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesBySourceType(Id sourceId, int type) const;

  /**
   * @brief Returns edges by their source IDs and type
   * @param sourceIds The IDs of the source nodes
   * @param type The type of the edges
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesBySourcesType(const std::vector<Id>& sourceIds, int type) const;

  /**
   * @brief Returns edges by their target ID and type
   * @param targetId The ID of the target node
   * @param type The type of the edges
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesByTargetType(Id targetId, int type) const;

  /**
   * @brief Returns edges by their target ID and type
   * @param targetId The ID of the target node
   * @param type The type of the edges
   * @return The edges
   */
  std::vector<StorageEdge> getEdgesByTargetsType(const std::vector<Id>& targetIds, int type) const;

  /**
   * @brief Returns a node by its ID
   * @param id The ID of the node to return
   * @return The node
   */
  StorageNode getNodeById(Id id) const;

  /**
   * @brief Returns a node by its serialized name
   * @param serializedName The serialized name of the node to return
   * @return The node
   */
  StorageNode getNodeBySerializedName(const std::wstring& serializedName) const;

  /**
   * @brief Returns the available node types
   * @return The available node types
   */
  std::vector<int> getAvailableNodeTypes() const;

  /**
   * @brief Returns the available edge types
   * @return The available edge types
   */
  std::vector<int> getAvailableEdgeTypes() const;

  /**
   * @brief Returns a file by its path
   * @param filePath The path of the file to return
   * @return The file
   */
  StorageFile getFileByPath(const std::wstring& filePath) const;

  /**
   * @brief Returns files by their paths
   * @param filePaths The paths of the files to return
   * @return The files
   */
  std::vector<StorageFile> getFilesByPaths(const std::vector<FilePath>& filePaths) const;

  /**
   * @brief Returns the content of a file by its path
   * @param filePath The path of the file to return
   * @return The content of the file
   */
  std::shared_ptr<TextAccess> getFileContentByPath(const std::wstring& filePath) const;

  /**
   * @brief Returns the content of a file by its ID
   * @param fileId The ID of the file to return
   * @return The content of the file
   */
  std::shared_ptr<TextAccess> getFileContentById(Id fileId) const;

  /**
   * @brief Sets the indexed status of a file
   * @param fileId The ID of the file to set
   * @param indexed The indexed status to set
   */
  void setFileIndexed(Id fileId, bool indexed);

  /**
   * @brief Sets the complete status of a file
   * @param fileId The ID of the file to set
   * @param filePath The path of the file to set
   * @param complete The complete status to set
   */
  void setFileCompleteIfNoError(Id fileId, const std::wstring& filePath, bool complete);

  /**
   * @brief Sets the node type
   * @param type The type of the node to set
   * @param nodeId The ID of the node to set
   */
  void setNodeType(int type, Id nodeId);

  /**
   * @brief Returns the source locations for a file
   * @param filePath The path of the file to return
   * @param query The query to return
   * @return The source locations
   */
  std::shared_ptr<SourceLocationFile> getSourceLocationsForFile(const FilePath& filePath, const std::string& query = "") const;

  /**
   * @brief Returns the source locations for lines in a file
   * @param filePath The path of the file to return
   * @param startLine The start line to return
   * @param endLine The end line to return
   * @return The source locations
   */
  std::shared_ptr<SourceLocationFile> getSourceLocationsForLinesInFile(const FilePath& filePath,
                                                                       size_t startLine,
                                                                       size_t endLine) const;

  /**
   * @brief Returns the source locations for a type in a file
   * @param filePath The path of the file to return
   * @param type The type of the source locations to return
   * @return The source locations
   */
  std::shared_ptr<SourceLocationFile> getSourceLocationsOfTypeInFile(const FilePath& filePath, LocationType type) const;

  /**
   * @brief Returns the source locations for element IDs
   * @param elementIds The IDs of the elements to return
   * @return The source locations
   */
  std::shared_ptr<SourceLocationCollection> getSourceLocationsForElementIds(const std::vector<Id>& elementIds) const;

  /**
   * @brief Returns the occurrences for a location ID
   * @param locationId The ID of the location to return
   * @return The occurrences
   */
  std::vector<StorageOccurrence> getOccurrencesForLocationId(Id locationId) const;

  /**
   * @brief Returns the occurrences for location IDs
   * @param locationIds The IDs of the locations to return
   * @return The occurrences
   */
  std::vector<StorageOccurrence> getOccurrencesForLocationIds(const std::vector<Id>& locationIds) const;

  /**
   * @brief Returns the occurrences for element IDs
   * @param elementIds The IDs of the elements to return
   * @return The occurrences
   */
  std::vector<StorageOccurrence> getOccurrencesForElementIds(const std::vector<Id>& elementIds) const;

  /**
   * @brief Returns a component access by node ID
   * @param nodeId The ID of the node to return
   * @return The component access
   */
  StorageComponentAccess getComponentAccessByNodeId(Id nodeId) const;

  /**
   * @brief Returns component accesses by node IDs
   * @param nodeIds The IDs of the nodes to return
   * @return The component accesses
   */
  std::vector<StorageComponentAccess> getComponentAccessesByNodeIds(const std::vector<Id>& nodeIds) const;

  /**
   * @brief Returns element components by element IDs
   * @param elementIds The IDs of the elements to return
   * @return The element components
   */
  std::vector<StorageElementComponent> getElementComponentsByElementIds(const std::vector<Id>& elementIds) const;

  /**
   * @brief Returns all error infos
   * @return The error infos
   */
  std::vector<ErrorInfo> getAllErrorInfos() const;

  /**
   * @brief Returns all elements
   * @return The elements
   */
  template <typename ResultType>
  std::vector<ResultType> getAll() const {
    return doGetAll<ResultType>("");
  }

  /**
   * @brief Returns the first element by its ID
   * @param id The ID of the element to return
   * @return The element
   */
  template <typename ResultType>
  ResultType getFirstById(const Id id) const {
    if(id != 0) {
      return doGetFirst<ResultType>("WHERE id == " + std::to_string(id));
    }
    return ResultType();
  }

  /**
   * @brief Returns all elements by their IDs
   * @param ids The IDs of the elements to return
   * @return The elements
   */
  template <typename ResultType>
  std::vector<ResultType> getAllByIds(const std::vector<Id>& ids) const {
    if(!ids.empty()) {
      return doGetAll<ResultType>("WHERE id IN (" + utility::join(utility::toStrings(ids), ',') + ")");
    }
    return std::vector<ResultType>();
  }

  /**
   * @brief Iterates over all elements
   * @param func The function to iterate over
   */
  template <typename StorageType>
  void forEach(std::function<void(StorageType&&)> func) const {
    forEach("", func);
  }

  /**
   * @brief Iterates over all elements of a specific type
   * @param type The type of the elements to iterate over
   * @param func The function to iterate over
   */
  template <typename StorageType>
  void forEachOfType(int type, std::function<void(StorageType&&)> func) const {
    forEach("WHERE type == " + std::to_string(type), func);
  }

  /**
   * @brief Iterates over all elements by their IDs
   * @param ids The IDs of the elements to iterate over
   * @param func The function to iterate over
   */
  template <typename StorageType>
  void forEachByIds(const std::vector<Id>& ids, std::function<void(StorageType&&)> func) const {
    if(!ids.empty()) {
      forEach("WHERE id IN (" + utility::join(utility::toStrings(ids), ',') + ")", func);
    }
  }

  /**
   * @brief Returns the number of nodes in the storage
   * @return The number of nodes
   */
  int getNodeCount() const;

  /**
   * @brief Returns the number of edges in the storage
   * @return The number of edges
   */
  int getEdgeCount() const;

  /**
   * @brief Returns the number of files in the storage
   * @return The number of files
   */
  int getFileCount() const;

  /**
   * @brief Returns the number of completed files in the storage
   * @return The number of completed files
   */
  int getCompletedFileCount() const;

  /**
   * @brief Returns the total number of lines in all files in the storage
   * @return The total number of lines
   */
  int getFileLineSum() const;

  /**
   * @brief Returns the number of source locations in the storage
   * @return The number of source locations
   */
  int getSourceLocationCount() const;

  /**
   * @brief Returns the number of errors in the storage
   * @return The number of errors
   */
  int getErrorCount() const;

private:
  static const size_t sStorageVersion;

  struct TempSourceLocation {
    TempSourceLocation(uint32_t startLine_, uint16_t lineDiff_, uint16_t startCol_, uint16_t endCol_, uint8_t type_)
        : startLine(startLine_), lineDiff(lineDiff_), startCol(startCol_), endCol(endCol_), type(type_) {}

    bool operator<(const TempSourceLocation& other) const {
      if(startLine != other.startLine) {
        return startLine < other.startLine;
      } else if(lineDiff != other.lineDiff) {
        return lineDiff < other.lineDiff;
      } else if(startCol != other.startCol) {
        return startCol < other.startCol;
      } else if(endCol != other.endCol) {
        return endCol < other.endCol;
      } else {
        return type < other.type;
      }
    }

    uint32_t startLine;
    uint16_t lineDiff;
    uint16_t startCol;
    uint16_t endCol;
    uint8_t type;
  };

  std::vector<std::pair<int, SqliteDatabaseIndex>> getIndices() const;

  void clearTables() override;
  void setupTables() override;
  void setupPrecompiledStatements() override;

  template <typename ResultType>
  std::vector<ResultType> doGetAll(const std::string& query) const {
    std::vector<ResultType> elements;
    forEach<ResultType>(query, [&elements](ResultType&& element) { elements.emplace_back(element); });
    return elements;
  }

  template <typename ResultType>
  ResultType doGetFirst(const std::string& query) const {
    std::vector<ResultType> results = doGetAll<ResultType>(query + " LIMIT 1");
    if(!results.empty()) {
      return results[0];
    }
    return ResultType();
  }

  template <typename StorageType>
  void forEach(const std::string& query, std::function<void(StorageType&&)> func) const;

  LowMemoryStringMap<std::string, uint32_t, 0> m_tempNodeNameIndex;
  LowMemoryStringMap<std::wstring, uint32_t, 0> m_tempWNodeNameIndex;
  std::map<uint32_t, int> m_tempNodeTypes;
  std::map<StorageEdgeData, uint32_t> m_tempEdgeIndex;
  std::map<std::wstring, std::map<std::wstring, uint32_t>> m_tempLocalSymbolIndex;
  std::map<uint32_t, std::map<TempSourceLocation, uint32_t>> m_tempSourceLocationIndices;

  template <typename StorageType>
  class InsertBatchStatement {
  public:
    void compile(const std::string& header,
                 size_t valueCount,
                 std::function<void(CppSQLite3Statement& stmt, const StorageType&, size_t)> bindValuesFunc,
                 CppSQLite3DB& database) {
      m_bindValuesFunc = bindValuesFunc;

      std::string valueStr = '(' + utility::join(std::vector<std::string>(valueCount, "?"), ',') + ')';

      const size_t MAX_VARIABLE_COUNT = 999;
      size_t batchSize = MAX_VARIABLE_COUNT / valueCount;

      while(true) {
        std::stringstream stmt;
        stmt << header;

        for(size_t i = 0; i < batchSize; i++) {
          if(i != 0) {
            stmt << ',';
          }
          stmt << valueStr;
        }
        stmt << ';';

        m_stmts.emplace_back(batchSize, database.compileStatement(stmt.str().c_str()));

        if(batchSize == 1) {
          break;
        } else {
          batchSize /= 2;
        }
      }
    }

    bool execute(const std::vector<StorageType>& types, SqliteIndexStorage* storage) {
      size_t i = 0;
      for(std::pair<size_t, CppSQLite3Statement>& p : m_stmts) {
        const size_t& batchSize = p.first;
        CppSQLite3Statement& stmt = p.second;

        while(types.size() - i >= batchSize) {
          for(size_t j = 0; j < batchSize; j++) {
            m_bindValuesFunc(stmt, types[i + j], j);
          }

          const bool success = storage->executeStatement(stmt);
          if(!success) {
            return false;
          }

          i += batchSize;
        }
      }

      return true;
    }

  private:
    std::vector<std::pair<size_t, CppSQLite3Statement>> m_stmts;

    std::function<void(CppSQLite3Statement& stmt, const StorageType&, size_t)> m_bindValuesFunc;
  };

  InsertBatchStatement<StorageNode> m_insertNodeBatchStatement;
  InsertBatchStatement<StorageEdge> m_insertEdgeBatchStatement;
  InsertBatchStatement<StorageSymbol> m_insertSymbolBatchStatement;
  InsertBatchStatement<StorageLocalSymbol> m_insertLocalSymbolBatchStatement;
  InsertBatchStatement<StorageSourceLocationData> m_insertSourceLocationBatchStatement;
  InsertBatchStatement<StorageOccurrence> m_insertOccurrenceBatchStatement;
  InsertBatchStatement<StorageComponentAccess> m_insertComponentAccessBatchStatement;

  CppSQLite3Statement m_insertElementStmt;
  CppSQLite3Statement m_insertElementComponentStmt;
  CppSQLite3Statement m_insertFileStmt;
  CppSQLite3Statement m_insertFileContentStmt;
  CppSQLite3Statement m_checkErrorExistsStmt;
  CppSQLite3Statement m_insertErrorStmt;
};

template <>
void SqliteIndexStorage::forEach<StorageEdge>(const std::string& query, std::function<void(StorageEdge&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageNode>(const std::string& query, std::function<void(StorageNode&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageSymbol>(const std::string& query, std::function<void(StorageSymbol&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageFile>(const std::string& query, std::function<void(StorageFile&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageLocalSymbol>(const std::string& query, std::function<void(StorageLocalSymbol&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageSourceLocation>(const std::string& query,
                                                        std::function<void(StorageSourceLocation&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageOccurrence>(const std::string& query, std::function<void(StorageOccurrence&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageComponentAccess>(const std::string& query,
                                                         std::function<void(StorageComponentAccess&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageElementComponent>(const std::string& query,
                                                          std::function<void(StorageElementComponent&&)> func) const;
template <>
void SqliteIndexStorage::forEach<StorageError>(const std::string& query, std::function<void(StorageError&&)> func) const;