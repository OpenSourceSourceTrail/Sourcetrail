#include "SqliteIndexStorage.h"

#include "FileSystem.h"
#include "GlobalId.hpp"
#include "LocationType.h"
#include "logging.h"
#include "SourceLocationCollection.h"
#include "SourceLocationFile.h"
#include "TextAccess.h"
#include "utilityString.h"

const size_t SqliteIndexStorage::sStorageVersion = 25;

namespace {
std::pair<std::wstring, std::wstring> splitLocalSymbolName(const std::wstring& name) {
  const auto pos = name.find_last_of(L'<');
  if(pos == std::wstring::npos || name.back() != L'>') {
    return {};
  }

  return std::make_pair(name.substr(0, pos), name.substr(pos + 1, name.size() - pos - 2));
}
}    // namespace

size_t SqliteIndexStorage::getStorageVersion() {
  return sStorageVersion;
}

SqliteIndexStorage::SqliteIndexStorage() = default;

SqliteIndexStorage::SqliteIndexStorage(const FilePath& dbFilePath) : SqliteStorage(dbFilePath.getCanonical()) {}

size_t SqliteIndexStorage::getStaticVersion() const {
  return sStorageVersion;
}

void SqliteIndexStorage::setMode(StorageModeType mode) {
  m_tempNodeNameIndex.clear();
  m_tempWNodeNameIndex.clear();
  m_tempNodeTypes.clear();
  m_tempEdgeIndex.clear();
  m_tempLocalSymbolIndex.clear();
  m_tempSourceLocationIndices.clear();

  std::vector<std::pair<int, SqliteDatabaseIndex>> indices = getIndices();
  for(auto& [index, databaseIndex] : indices) {
    if(index & mode) {
      std::ignore = databaseIndex.createOnDatabase(m_database);
    } else {
      std::ignore = databaseIndex.removeFromDatabase(m_database);
    }
  }
}

std::string SqliteIndexStorage::getProjectSettingsText() const {
  return getMetaValue("project_settings");
}

void SqliteIndexStorage::setProjectSettingsText(const std::string& text) {
  insertOrUpdateMetaValue("project_settings", text);
}

Id SqliteIndexStorage::addNode(const StorageNodeData& data) {
  std::vector<Id> ids = addNodes({StorageNode(0, data)});
  return ids.empty() ? 0 : ids.front();
}

std::vector<Id> SqliteIndexStorage::addNodes(const std::vector<StorageNode>& nodes) {
  if(m_tempNodeNameIndex.empty() && m_tempWNodeNameIndex.empty()) {
    forEach<StorageNode>([this](StorageNode&& node) {
      const std::string name = utility::encodeToUtf8(node.serializedName);
      if(name.size() != node.serializedName.size()) {
        m_tempWNodeNameIndex.add(node.serializedName, static_cast<uint32_t>(node.id));
      } else {
        m_tempNodeNameIndex.add(name, static_cast<uint32_t>(node.id));
      }

      m_tempNodeTypes.emplace(static_cast<uint32_t>(node.id), node.type);
    });
  }

  std::vector<Id> nodeIds(nodes.size(), 0);
  std::vector<StorageNode> nodesToInsert;
  for(size_t i = 0; i < nodes.size(); i++) {
    const StorageNodeData& data = nodes[i];
    const std::string name = utility::encodeToUtf8(data.serializedName);
    {
      Id nodeId = 0U;
      if(name.size() != data.serializedName.size()) {
        nodeId = m_tempWNodeNameIndex.find(data.serializedName);
      } else {
        nodeId = m_tempNodeNameIndex.find(name);
      }

      if(0U != nodeId) {
        if(auto iterator = m_tempNodeTypes.find(static_cast<uint32_t>(nodeId));
           iterator != m_tempNodeTypes.end() && iterator->second < data.type) {
          setNodeType(data.type, nodeId);
          m_tempNodeTypes[static_cast<uint32_t>(nodeId)] = data.type;
        }

        nodeIds[i] = nodeId;
      } else {
        executeStatement(m_insertElementStmt);
        const Id lastRowId = static_cast<Id>(m_database.lastRowId());

        nodesToInsert.emplace_back(lastRowId, data);
        nodeIds[i] = lastRowId;

        if(name.size() != data.serializedName.size()) {
          m_tempWNodeNameIndex.add(data.serializedName, static_cast<uint32_t>(lastRowId));
        } else {
          m_tempNodeNameIndex.add(name, static_cast<uint32_t>(lastRowId));
        }
        m_tempNodeTypes.emplace(static_cast<uint32_t>(lastRowId), data.type);
      }
    }
  }

  if(!nodesToInsert.empty()) {
    m_insertNodeBatchStatement.execute(nodesToInsert, this);
  }

  return nodeIds;
}

bool SqliteIndexStorage::addSymbol(const StorageSymbol& data) {
  return addSymbols({data});
}

bool SqliteIndexStorage::addSymbols(const std::vector<StorageSymbol>& symbols) {
  return m_insertSymbolBatchStatement.execute(symbols, this);
}

bool SqliteIndexStorage::addFile(const StorageFile& data) {
  if(getFileByPath(data.filePath).id != 0) {
    return false;
  }

  const FilePath filePath(data.filePath);

  std::string modificationTime(data.modificationTime);
  if(modificationTime.empty()) {
    modificationTime = FileSystem::getFileInfoForPath(filePath).lastWriteTime.toString();
  }

  std::shared_ptr<TextAccess> content;
  int lineCount = 0;
  if(data.indexed) {
    content = TextAccess::createFromFile(filePath);
    lineCount = static_cast<int>(content->getLineCount());
  }

  bool success = false;
  {
    m_insertFileStmt.bind(1, int(data.id));
    m_insertFileStmt.bind(2, utility::encodeToUtf8(data.filePath).c_str());
    m_insertFileStmt.bind(3, utility::encodeToUtf8(data.languageIdentifier).c_str());
    m_insertFileStmt.bind(4, modificationTime.c_str());
    m_insertFileStmt.bind(5, data.indexed ? 1 : 0);
    m_insertFileStmt.bind(6, data.complete ? 1 : 0);
    m_insertFileStmt.bind(7, lineCount);
    success = executeStatement(m_insertFileStmt);
  }

  if(success && content) {
    m_insertFileContentStmt.bind(1, int(data.id));
    m_insertFileContentStmt.bind(2, content->getText().c_str());
    success = executeStatement(m_insertFileContentStmt);
  }

  return success;
}

Id SqliteIndexStorage::addEdge(const StorageEdgeData& data) {
  const std::vector<Id> ids = addEdges({StorageEdge(0, data)});
  return ids.empty() ? 0 : ids.front();
}

std::vector<Id> SqliteIndexStorage::addEdges(const std::vector<StorageEdge>& edges) {
  if(m_tempEdgeIndex.empty()) {
    forEach<StorageEdge>([this](StorageEdge&& edge) {
      m_tempEdgeIndex.emplace(StorageEdgeData(edge.type, edge.sourceNodeId, edge.targetNodeId), static_cast<uint32_t>(edge.id));
    });
  }

  std::vector<Id> edgeIds(edges.size(), 0);
  std::vector<StorageEdge> edgesToInsert;
  for(size_t i = 0; i < edges.size(); i++) {
    const StorageEdge& data = edges[i];
    if(auto iterator = m_tempEdgeIndex.find(data); iterator != m_tempEdgeIndex.end()) {
      edgeIds[i] = iterator->second;
    } else {
      executeStatement(m_insertElementStmt);
      const Id lastRowId = static_cast<Id>(m_database.lastRowId());

      edgeIds[i] = lastRowId;
      edgesToInsert.emplace_back(lastRowId, data);

      m_tempEdgeIndex.emplace(data, static_cast<uint32_t>(lastRowId));
    }
  }

  if(!edgesToInsert.empty()) {
    m_insertEdgeBatchStatement.execute(edgesToInsert, this);
  }

  return edgeIds;
}

Id SqliteIndexStorage::addLocalSymbol(const StorageLocalSymbolData& data) {
  const std::vector<Id> ids = addLocalSymbols({StorageLocalSymbol(0, data)});
  return ids.empty() ? 0 : ids.front();
}

std::vector<Id> SqliteIndexStorage::addLocalSymbols(const std::set<StorageLocalSymbol>& symbols) {
  if(m_tempLocalSymbolIndex.empty()) {
    forEach<StorageLocalSymbol>([this](StorageLocalSymbol&& localSymbol) {
      const auto& [name, symbol] = splitLocalSymbolName(localSymbol.name);
      if(!symbol.empty()) {
        m_tempLocalSymbolIndex[name].emplace(symbol, static_cast<uint32_t>(localSymbol.id));
      }
    });
  }

  std::vector<Id> symbolIds(symbols.size(), 0);
  std::vector<StorageLocalSymbol> symbolsToInsert;
  auto iterator = symbols.begin();
  for(size_t i = 0; i < symbols.size(); i++) {
    const StorageLocalSymbol& data = *iterator;
    std::pair<std::wstring, std::wstring> name = splitLocalSymbolName(data.name);
    if(!name.second.empty()) {
      auto found = m_tempLocalSymbolIndex.find(name.first);
      if(found != m_tempLocalSymbolIndex.end()) {
        auto it2 = found->second.find(name.second);
        if(it2 != found->second.end()) {
          symbolIds[i] = it2->second;
        }
      }
    }

    if(0U == symbolIds[i]) {
      std::ignore = executeStatement(m_insertElementStmt);
      const Id lastRowId = static_cast<Id>(m_database.lastRowId());

      symbolIds[i] = lastRowId;
      symbolsToInsert.emplace_back(lastRowId, data);
      if(!name.second.empty()) {
        m_tempLocalSymbolIndex[name.first].emplace(name.second, static_cast<uint32_t>(lastRowId));
      }
    }

    iterator++;
  }

  if(!symbolsToInsert.empty()) {
    m_insertLocalSymbolBatchStatement.execute(symbolsToInsert, this);
  }

  return symbolIds;
}

Id SqliteIndexStorage::addSourceLocation(const StorageSourceLocationData& data) {
  const std::vector<Id> ids = addSourceLocations({StorageSourceLocation(0, data)});
  return ids.empty() ? 0 : ids.front();
}

std::vector<Id> SqliteIndexStorage::addSourceLocations(const std::vector<StorageSourceLocation>& locations) {
  if(m_tempSourceLocationIndices.empty()) {
    forEach<StorageSourceLocation>([this](StorageSourceLocation&& loc) {
      std::map<TempSourceLocation, uint32_t>& index = m_tempSourceLocationIndices[static_cast<uint32_t>(loc.fileNodeId)];
      index.emplace(TempSourceLocation(static_cast<uint32_t>(loc.startLine),
                                       static_cast<uint16_t>(loc.endLine - loc.startLine),
                                       static_cast<uint16_t>(loc.startCol),
                                       static_cast<uint16_t>(loc.endCol),
                                       static_cast<uint8_t>(loc.type)),
                    static_cast<uint32_t>(loc.id));
    });
  }

  std::vector<Id> locationIds(locations.size(), 0);
  std::vector<StorageSourceLocationData> locationsToInsert;
  const size_t lastRowId = static_cast<size_t>(executeStatementScalar("SELECT MAX(rowid) from source_location", 0));

  for(size_t i = 0; i < locations.size(); i++) {
    const StorageSourceLocation& data = locations[i];
    const TempSourceLocation tempLoc(static_cast<uint32_t>(data.startLine),
                                     static_cast<uint16_t>(data.endLine - data.startLine),
                                     static_cast<uint16_t>(data.startCol),
                                     static_cast<uint16_t>(data.endCol),
                                     static_cast<uint8_t>(data.type));

    std::map<TempSourceLocation, uint32_t>& index = m_tempSourceLocationIndices[static_cast<uint32_t>(data.fileNodeId)];
    if(auto iterator = index.find(tempLoc); iterator != index.end()) {
      locationIds[i] = iterator->second;
    } else {
      std::ignore = executeStatement(m_insertElementStmt);
      Id newId = lastRowId + 1 + locationsToInsert.size();

      locationIds[i] = newId;
      index.emplace(tempLoc, static_cast<uint32_t>(newId));

      locationsToInsert.emplace_back(data);
    }
  }

  if(!locationsToInsert.empty()) {
    m_insertSourceLocationBatchStatement.execute(locationsToInsert, this);
  }

  return locationIds;
}

bool SqliteIndexStorage::addOccurrence(const StorageOccurrence& data) {
  return addOccurrences({data});
}

bool SqliteIndexStorage::addOccurrences(const std::vector<StorageOccurrence>& occurrences) {
  return m_insertOccurrenceBatchStatement.execute(occurrences, this);
}

bool SqliteIndexStorage::addComponentAccess(const StorageComponentAccess& componentAccess) {
  return addComponentAccesses({componentAccess});
}

bool SqliteIndexStorage::addComponentAccesses(const std::vector<StorageComponentAccess>& componentAccesses) {
  return m_insertComponentAccessBatchStatement.execute(componentAccesses, this);
}

void SqliteIndexStorage::addElementComponent(const StorageElementComponent& component) {
  m_insertElementComponentStmt.bind(1, int(component.elementId));
  m_insertElementComponentStmt.bind(2, component.type);
  m_insertElementComponentStmt.bind(3, utility::encodeToUtf8(component.data).c_str());
  executeStatement(m_insertElementComponentStmt);
  m_insertElementComponentStmt.reset();
}

void SqliteIndexStorage::addElementComponents(const std::vector<StorageElementComponent>& components) {
  for(const StorageElementComponent& component : components) {
    addElementComponent(component);
  }
}

StorageError SqliteIndexStorage::addError(const StorageErrorData& data) {
  const std::wstring sanitizedMessage = utility::replace(data.message, L"'", L"''");

  Id lastRowId = 0;
  {
    m_checkErrorExistsStmt.bind(1, utility::encodeToUtf8(sanitizedMessage).c_str());
    m_checkErrorExistsStmt.bind(2, int(data.fatal));

    CppSQLite3Query checkQuery = executeQuery(m_checkErrorExistsStmt);
    if(!checkQuery.eof() && checkQuery.numFields() > 0) {
      lastRowId = static_cast<Id>(checkQuery.getIntField(0, -1));
    }
    m_checkErrorExistsStmt.reset();
  }

  if(0 == lastRowId) {
    executeStatement(m_insertElementStmt);
    lastRowId = static_cast<Id>(m_database.lastRowId());

    m_insertErrorStmt.bind(1, int(lastRowId));
    m_insertErrorStmt.bind(2, utility::encodeToUtf8(sanitizedMessage).c_str());
    m_insertErrorStmt.bind(3, static_cast<int>(data.fatal));
    m_insertErrorStmt.bind(4, static_cast<int>(data.indexed));
    m_insertErrorStmt.bind(5, utility::encodeToUtf8(data.translationUnit).c_str());

    const bool success = executeStatement(m_insertErrorStmt);
    if(success) {
      lastRowId = static_cast<Id>(m_database.lastRowId());
    }
  }

  return {lastRowId, data};
}

void SqliteIndexStorage::removeElement(Id elementId) {
  std::vector<Id> ids;
  ids.push_back(elementId);
  removeElements(ids);
}

void SqliteIndexStorage::removeElements(const std::vector<Id>& ids) {
  executeStatement("DELETE FROM element WHERE id IN (" + utility::join(utility::toStrings(ids), ',') + ");");
}

void SqliteIndexStorage::removeOccurrence(const StorageOccurrence& occurrence) {
  executeStatement("DELETE FROM occurrence WHERE element_id = " + std::to_string(occurrence.elementId) +
                   " AND source_location_id = " + std::to_string(occurrence.sourceLocationId) + ";");
}

void SqliteIndexStorage::removeOccurrences(const std::vector<StorageOccurrence>& occurrences) {
  for(const StorageOccurrence& occurrence : occurrences) {
    removeOccurrence(occurrence);
  }
}

void SqliteIndexStorage::removeElementsWithoutOccurrences(const std::vector<Id>& elementIds) {
  executeStatement("DELETE FROM element WHERE id IN (" + utility::join(utility::toStrings(elementIds), ',') +
                   ") AND id NOT IN (SELECT element_id FROM occurrence);");
}

void SqliteIndexStorage::removeElementsWithLocationInFiles(const std::vector<Id>& fileIds,
                                                           const std::function<void(int)>& updateStatusCallback) {
  if(updateStatusCallback != nullptr) {
    updateStatusCallback(1);
  }

  // preparing
  executeStatement("DROP TABLE IF EXISTS main.element_id_to_clear;");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(2);
  }

  executeStatement(
      "CREATE TABLE IF NOT EXISTS element_id_to_clear("
      "id INTEGER NOT NULL, "
      "PRIMARY KEY(id));");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(3);
  }

  // store ids of all elements located in fileIds into element_id_to_clear
  executeStatement(
      "INSERT INTO element_id_to_clear "
      "	SELECT occurrence.element_id "
      "	FROM occurrence "
      "	INNER JOIN source_location ON ("
      "		occurrence.source_location_id = source_location.id"
      "	) "
      "	WHERE source_location.file_node_id IN (" +
      utility::join(utility::toStrings(fileIds), ',') +
      ")"
      "	GROUP BY (occurrence.element_id)");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(4);
  }

  // delete all edges in element_id_to_clear
  executeStatement(
      "DELETE FROM element WHERE element.id IN "
      "	(SELECT element_id_to_clear.id FROM element_id_to_clear INNER JOIN edge ON "
      "(element_id_to_clear.id = edge.id))");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(30);
  }

  // delete all edges originating from element_id_to_clear
  executeStatement(
      "DELETE FROM element WHERE element.id IN (SELECT id FROM edge WHERE source_node_id IN "
      "(SELECT id FROM element_id_to_clear))");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(31);
  }

  // remove all non existing ids from element_id_to_clear (they have been cleared by now and we
  // can disregard them)
  executeStatement(
      "DELETE FROM element_id_to_clear WHERE id NOT IN ("
      "	SELECT id FROM element"
      ")");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(33);
  }

  // remove all files from element_id_to_clear (they will be cleared later)
  executeStatement(
      "DELETE FROM element_id_to_clear WHERE id IN ("
      "	SELECT id FROM file"
      ")");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(34);
  }

  // delete source locations from fileIds (this also deletes the respective occurrences)
  executeStatement("DELETE FROM source_location WHERE file_node_id IN (" + utility::join(utility::toStrings(fileIds), ',') + ");");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(45);
  }

  // remove all ids from element_id_to_clear that still have occurrences
  executeStatement(
      "DELETE FROM element_id_to_clear WHERE id IN ("
      "	SELECT element_id_to_clear.id FROM element_id_to_clear INNER JOIN occurrence ON "
      "		element_id_to_clear.id = occurrence.element_id"
      ")");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(59);
  }

  // remove all ids from element_id_to_clear that still have an edge pointing to them
  executeStatement(
      "DELETE FROM element_id_to_clear WHERE id IN ("
      "	SELECT target_node_id FROM edge"
      ")");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(74);
  }

  // delete all elements that are still listed in element_id_to_clear
  executeStatement(
      "DELETE FROM element WHERE EXISTS ("
      "	SELECT * FROM element_id_to_clear WHERE element.id = element_id_to_clear.id"
      ")");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(87);
  }

  // cleaning up
  executeStatement("DROP TABLE IF EXISTS main.element_id_to_clear;");

  if(updateStatusCallback != nullptr) {
    updateStatusCallback(89);
  }
}

void SqliteIndexStorage::removeAllErrors() {
  executeStatement("DELETE FROM error;");
}

bool SqliteIndexStorage::isEdge(Id elementId) const {
  return executeStatementScalar("SELECT count(*) FROM edge WHERE id = " + std::to_string(elementId) + ";", 0) > 0;
}

bool SqliteIndexStorage::isNode(Id elementId) const {
  return executeStatementScalar("SELECT count(*) FROM node WHERE id = " + std::to_string(elementId) + ";", 0) > 0;
}

bool SqliteIndexStorage::isFile(Id elementId) const {
  return executeStatementScalar("SELECT count(*) FROM file WHERE id = " + std::to_string(elementId) + ";", 0) > 0;
}

StorageEdge SqliteIndexStorage::getEdgeById(Id edgeId) const {
  const std::vector<StorageEdge> candidates = doGetAll<StorageEdge>("WHERE id = " + std::to_string(edgeId));

  return candidates.empty() ? StorageEdge{} : candidates.front();
}

StorageEdge SqliteIndexStorage::getEdgeBySourceTargetType(Id sourceId, Id targetId, int type) const {
  return doGetFirst<StorageEdge>(
      "WHERE "
      "source_node_id == " +
      std::to_string(sourceId) +
      " AND "
      "target_node_id == " +
      std::to_string(targetId) +
      " AND "
      "type == " +
      std::to_string(type));
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesBySourceId(Id sourceId) const {
  return doGetAll<StorageEdge>("WHERE source_node_id == " + std::to_string(sourceId));
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesBySourceIds(const std::vector<Id>& sourceIds) const {
  return doGetAll<StorageEdge>("WHERE source_node_id IN (" + utility::join(utility::toStrings(sourceIds), ',') + ")");
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesByTargetId(Id targetId) const {
  return doGetAll<StorageEdge>("WHERE target_node_id == " + std::to_string(targetId));
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesByTargetIds(const std::vector<Id>& targetIds) const {
  return doGetAll<StorageEdge>("WHERE target_node_id IN (" + utility::join(utility::toStrings(targetIds), ',') + ")");
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesBySourceOrTargetId(Id targetId) const {
  return doGetAll<StorageEdge>("WHERE source_node_id == " + std::to_string(targetId) +
                               " OR target_node_id == " + std::to_string(targetId));
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesByType(int type) const {
  return doGetAll<StorageEdge>("WHERE type == " + std::to_string(type));
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesBySourceType(Id sourceId, int type) const {
  return doGetAll<StorageEdge>("WHERE source_node_id == " + std::to_string(sourceId) + " AND type == " + std::to_string(type));
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesBySourcesType(const std::vector<Id>& sourceIds, int type) const {
  return doGetAll<StorageEdge>("WHERE source_node_id IN (" + utility::join(utility::toStrings(sourceIds), ',') +
                               ")"
                               " AND type == " +
                               std::to_string(type));
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesByTargetType(Id targetId, int type) const {
  return doGetAll<StorageEdge>("WHERE target_node_id == " + std::to_string(targetId) + " AND type == " + std::to_string(type));
}

std::vector<StorageEdge> SqliteIndexStorage::getEdgesByTargetsType(const std::vector<Id>& targetIds, int type) const {
  return doGetAll<StorageEdge>("WHERE target_node_id IN (" + utility::join(utility::toStrings(targetIds), ',') +
                               ")"
                               " AND type == " +
                               std::to_string(type));
}

StorageNode SqliteIndexStorage::getNodeById(Id nodeId) const {
  const std::vector<StorageNode> candidates = doGetAll<StorageNode>("WHERE id = " + std::to_string(nodeId));

  return candidates.empty() ? StorageNode{} : candidates.front();
}

StorageNode SqliteIndexStorage::getNodeBySerializedName(const std::wstring& serializedName) const {
  CppSQLite3Statement stmt = m_database.compileStatement(
      "SELECT id, type, serialized_name FROM node WHERE serialized_name == ? LIMIT 1;");

  stmt.bind(1, utility::encodeToUtf8(serializedName).c_str());
  CppSQLite3Query query = executeQuery(stmt);

  if(!query.eof()) {
    const Id elementId = static_cast<Id>(query.getIntField(0, 0));
    const int type = query.getIntField(1, -1);
    const std::string name = query.getStringField(2, "");

    if(elementId != 0 && type != -1) {
      return {elementId, type, utility::decodeFromUtf8(name)};
    }
  }

  stmt.reset();

  return {};
}

std::vector<int> SqliteIndexStorage::getAvailableNodeTypes() const {
  CppSQLite3Query query = executeQuery("SELECT DISTINCT type FROM node;");

  std::vector<int> types;

  while(!query.eof()) {
    const int type = query.getIntField(0, -1);
    if(type != -1) {
      types.push_back(type);
    }

    query.nextRow();
  }

  return types;
}

std::vector<int> SqliteIndexStorage::getAvailableEdgeTypes() const {
  CppSQLite3Query query = executeQuery("SELECT DISTINCT type FROM edge;");

  std::vector<int> types;

  while(!query.eof()) {
    const int type = query.getIntField(0, -1);
    if(type != -1) {
      types.push_back(type);
    }

    query.nextRow();
  }

  return types;
}

StorageFile SqliteIndexStorage::getFileByPath(const std::wstring& filePath) const {
  return doGetFirst<StorageFile>("WHERE file.path == '" + utility::encodeToUtf8(filePath) + "'");
}

std::vector<StorageFile> SqliteIndexStorage::getFilesByPaths(const std::vector<FilePath>& filePaths) const {
  return doGetAll<StorageFile>("WHERE file.path IN ('" + utility::join(utility::toStrings(filePaths), "', '") + "')");
}

std::shared_ptr<TextAccess> SqliteIndexStorage::getFileContentById(Id fileId) const {
  CppSQLite3Query query = executeQuery("SELECT content FROM filecontent WHERE id = '" + std::to_string(fileId) + "';");
  if(!query.eof()) {
    return TextAccess::createFromString(query.getStringField(0, ""));
  }

  return TextAccess::createFromString("");
}

std::shared_ptr<TextAccess> SqliteIndexStorage::getFileContentByPath(const std::wstring& filePath) const {
  try {
    CppSQLite3Query query = executeQuery(
        "SELECT filecontent.content "
        "FROM filecontent "
        "INNER JOIN file ON filecontent.id = file.id "
        "WHERE file.path = '" +
        utility::encodeToUtf8(filePath) + "';");

    if(!query.eof()) {
      return TextAccess::createFromString(query.getStringField(0, ""));
    }
  } catch(CppSQLite3Exception& e) {
    LOG_ERROR(std::to_string(e.errorCode()) + ": " + e.errorMessage());
  }

  return TextAccess::createFromString("");
}

void SqliteIndexStorage::setFileIndexed(Id fileId, bool indexed) {
  executeStatement("UPDATE file SET indexed = " + std::to_string(indexed) + " WHERE id == " + std::to_string(fileId) + ";");
}

void SqliteIndexStorage::setFileCompleteIfNoError(Id fileId, const std::wstring& /*filePath*/, bool complete) {
  const bool fileHasErrors = doGetFirst<StorageSourceLocation>("WHERE file_node_id == " + std::to_string(fileId) +
                                                               " AND type == " + std::to_string(locationTypeToInt(LOCATION_ERROR)))
                                 .id != 0U;
  if(fileHasErrors != complete) {
    executeStatement("UPDATE file SET complete = " + std::to_string(complete) + " WHERE id == " + std::to_string(fileId) + ";");
  }
}

void SqliteIndexStorage::setNodeType(int type, Id nodeId) {
  executeStatement("UPDATE node SET type = " + std::to_string(type) + " WHERE id == " + std::to_string(nodeId) + ";");
}

std::shared_ptr<SourceLocationFile> SqliteIndexStorage::getSourceLocationsForFile(const FilePath& filePath,
                                                                                  const std::string& query) const {
  std::shared_ptr<SourceLocationFile> ret = std::make_shared<SourceLocationFile>(filePath, L"", true, false, false);

  const StorageFile file = getFileByPath(filePath.wstr());
  if(file.id == 0)    // early out
  {
    return ret;
  }

  ret->setLanguage(file.languageIdentifier);
  ret->setIsComplete(file.complete);
  ret->setIsIndexed(file.indexed);

  const std::vector<StorageSourceLocation> sourceLocations = doGetAll<StorageSourceLocation>(
      "WHERE file_node_id == " + std::to_string(file.id) + " " + query);

  std::vector<Id> sourceLocationIds;
  sourceLocationIds.reserve(sourceLocations.size());
  for(const StorageSourceLocation& storageLocation : sourceLocations) {
    sourceLocationIds.push_back(storageLocation.id);
  }

  std::map<Id, std::vector<Id>> sourceLocationIdToElementIds;
  for(const StorageOccurrence& occurrence : getOccurrencesForLocationIds(sourceLocationIds)) {
    sourceLocationIdToElementIds[occurrence.sourceLocationId].push_back(occurrence.elementId);
  }

  for(const StorageSourceLocation& location : sourceLocations) {
    auto iterator = sourceLocationIdToElementIds.find(location.id);

    ret->addSourceLocation(intToLocationType(location.type),
                           location.id,
                           iterator != sourceLocationIdToElementIds.end() ? iterator->second : std::vector<Id>(),
                           location.startLine,
                           location.startCol,
                           location.endLine,
                           location.endCol);
  }

  return ret;
}

std::shared_ptr<SourceLocationFile> SqliteIndexStorage::getSourceLocationsForLinesInFile(const FilePath& filePath,
                                                                                         size_t startLine,
                                                                                         size_t endLine) const {
  return getSourceLocationsForFile(
      filePath, "AND start_line <= " + std::to_string(endLine) + " AND end_line >= " + std::to_string(startLine));
}

std::shared_ptr<SourceLocationFile> SqliteIndexStorage::getSourceLocationsOfTypeInFile(const FilePath& filePath,
                                                                                       LocationType type) const {
  return getSourceLocationsForFile(filePath, "AND type == " + std::to_string(locationTypeToInt(type)));
}

std::shared_ptr<SourceLocationCollection> SqliteIndexStorage::getSourceLocationsForElementIds(const std::vector<Id>& elementIds) const {
  std::vector<Id> sourceLocationIds;
  std::map<Id, std::vector<Id>> sourceLocationIdToElementIds;
  for(const StorageOccurrence& occurrence : getOccurrencesForElementIds(elementIds)) {
    sourceLocationIds.push_back(occurrence.sourceLocationId);
    sourceLocationIdToElementIds[occurrence.sourceLocationId].push_back(occurrence.elementId);
  }

  CppSQLite3Query query = executeQuery(
      "SELECT source_location.id, file.path, source_location.start_line, "
      "source_location.start_column, "
      "source_location.end_line, source_location.end_column, source_location.type "
      "FROM source_location INNER JOIN file ON (file.id = source_location.file_node_id) "
      "WHERE source_location.id IN (" +
      utility::join(utility::toStrings(sourceLocationIds), ',') + ");");

  std::shared_ptr<SourceLocationCollection> ret = std::make_shared<SourceLocationCollection>();

  while(!query.eof()) {
    const Id elementId = static_cast<Id>(query.getIntField(0, 0));
    const std::string filePath = query.getStringField(1, "");
    const int startLineNumber = query.getIntField(2, -1);
    const int startColNumber = query.getIntField(3, -1);
    const int endLineNumber = query.getIntField(4, -1);
    const int endColNumber = query.getIntField(5, -1);
    const int type = query.getIntField(6, -1);

    if(elementId != 0 && !filePath.empty() && startLineNumber != -1 && startColNumber != -1 && endLineNumber != -1 &&
       endColNumber != -1 && type != -1) {
      ret->addSourceLocation(intToLocationType(type),
                             elementId,
                             sourceLocationIdToElementIds[elementId],
                             FilePath(utility::decodeFromUtf8(filePath)),
                             static_cast<size_t>(startLineNumber),
                             static_cast<size_t>(startColNumber),
                             static_cast<size_t>(endLineNumber),
                             static_cast<size_t>(endColNumber));
    }

    query.nextRow();
  }

  return ret;
}

std::vector<StorageOccurrence> SqliteIndexStorage::getOccurrencesForLocationId(Id locationId) const {
  return getOccurrencesForLocationIds({locationId});
}

std::vector<StorageOccurrence> SqliteIndexStorage::getOccurrencesForLocationIds(const std::vector<Id>& locationIds) const {
  return doGetAll<StorageOccurrence>("WHERE source_location_id IN (" + utility::join(utility::toStrings(locationIds), ',') + ")");
}

std::vector<StorageOccurrence> SqliteIndexStorage::getOccurrencesForElementIds(const std::vector<Id>& elementIds) const {
  return doGetAll<StorageOccurrence>("WHERE element_id IN (" + utility::join(utility::toStrings(elementIds), ',') + ")");
}

StorageComponentAccess SqliteIndexStorage::getComponentAccessByNodeId(Id nodeId) const {
  return doGetFirst<StorageComponentAccess>("WHERE node_id == " + std::to_string(nodeId));
}

std::vector<StorageComponentAccess> SqliteIndexStorage::getComponentAccessesByNodeIds(const std::vector<Id>& nodeIds) const {
  return doGetAll<StorageComponentAccess>("WHERE node_id IN (" + utility::join(utility::toStrings(nodeIds), ',') + ")");
}

std::vector<StorageElementComponent> SqliteIndexStorage::getElementComponentsByElementIds(const std::vector<Id>& elementIds) const {
  return doGetAll<StorageElementComponent>("WHERE element_id IN (" + utility::join(utility::toStrings(elementIds), ',') + ")");
}

std::vector<ErrorInfo> SqliteIndexStorage::getAllErrorInfos() const {
  std::vector<ErrorInfo> errorInfos;

  CppSQLite3Query query = executeQuery(
      "SELECT error.id, error.message, error.fatal, error.indexed, error.translation_unit, "
      "file.path, source_location.start_line, source_location.start_column "
      "FROM occurrence "
      "INNER JOIN error ON (error.id = occurrence.element_id) "
      "INNER JOIN source_location ON (source_location.id = occurrence.source_location_id) "
      "INNER JOIN file ON (file.id = source_location.file_node_id);");

  std::map<Id, size_t> errorIdCount;

  while(!query.eof()) {
    const Id elementId = static_cast<Id>(query.getIntField(0, 0));
    const std::string message = query.getStringField(1, "");
    const bool fatal = query.getIntField(2, 0) != 0;
    const bool indexed = query.getIntField(3, 0) != 0;
    const std::string translationUnit = query.getStringField(4, "");
    const std::string filePath = query.getStringField(5, "");
    const int lineNumber = query.getIntField(6, -1);
    const int columnNumber = query.getIntField(7, -1);

    if(elementId != 0) {
      // There can be multiple errors with the same id, so a count is added to the id
      Id errorId = elementId * 10000;
      if(auto iterator = errorIdCount.find(elementId); iterator != errorIdCount.end()) {
        errorId += iterator->second;
        iterator->second = iterator->second + 1;
      } else {
        errorIdCount.emplace(elementId, 1);
      }

      errorInfos.emplace_back(errorId,
                              utility::decodeFromUtf8(message),
                              utility::decodeFromUtf8(filePath),
                              lineNumber,
                              columnNumber,
                              utility::decodeFromUtf8(translationUnit),
                              fatal,
                              indexed);
    }

    query.nextRow();
  }

  return errorInfos;
}

int SqliteIndexStorage::getNodeCount() const {
  return executeStatementScalar("SELECT COUNT(*) FROM node;", 0);
}

int SqliteIndexStorage::getEdgeCount() const {
  return executeStatementScalar("SELECT COUNT(*) FROM edge;", 0);
}

int SqliteIndexStorage::getFileCount() const {
  return executeStatementScalar("SELECT COUNT(*) FROM file WHERE indexed = 1;", 0);
}

int SqliteIndexStorage::getCompletedFileCount() const {
  return executeStatementScalar("SELECT COUNT(*) FROM file WHERE indexed = 1 AND complete = 1;", 0);
}

int SqliteIndexStorage::getFileLineSum() const {
  return executeStatementScalar("SELECT SUM(line_count) FROM file;", 0);
}

int SqliteIndexStorage::getSourceLocationCount() const {
  return executeStatementScalar("SELECT COUNT(*) FROM source_location;", 0);
}

int SqliteIndexStorage::getErrorCount() const {
  return executeStatementScalar("SELECT COUNT(*) FROM error INNER JOIN occurrence ON (error.id = occurrence.element_id);", 0);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::vector<std::pair<int, SqliteDatabaseIndex>> SqliteIndexStorage::getIndices() const {
  std::vector<std::pair<int, SqliteDatabaseIndex>> indices;
  indices.emplace_back(STORAGE_MODE_CLEAR, SqliteDatabaseIndex("edge_source_node_id_index", "edge(source_node_id)"));
  indices.emplace_back(STORAGE_MODE_CLEAR, SqliteDatabaseIndex("edge_target_node_id_index", "edge(target_node_id)"));
  indices.emplace_back(
      STORAGE_MODE_READ | STORAGE_MODE_CLEAR, SqliteDatabaseIndex("node_serialized_name_index", "node(serialized_name)"));
  indices.emplace_back(STORAGE_MODE_READ | STORAGE_MODE_CLEAR,
                       SqliteDatabaseIndex("source_location_file_node_id_index", "source_location(file_node_id)"));
  indices.emplace_back(STORAGE_MODE_WRITE, SqliteDatabaseIndex("error_all_data_index", "error(message, fatal)"));
  indices.emplace_back(STORAGE_MODE_WRITE, SqliteDatabaseIndex("file_path_index", "file(path)"));
  indices.emplace_back(
      STORAGE_MODE_READ | STORAGE_MODE_CLEAR, SqliteDatabaseIndex("occurrence_element_id_index", "occurrence(element_id)"));
  indices.emplace_back(STORAGE_MODE_READ | STORAGE_MODE_CLEAR,
                       SqliteDatabaseIndex("occurrence_source_location_id_index", "occurrence(source_location_id)"));
  indices.emplace_back(
      STORAGE_MODE_CLEAR, SqliteDatabaseIndex("element_component_foreign_key_index", "element_component(element_id)"));
  indices.emplace_back(STORAGE_MODE_CLEAR, SqliteDatabaseIndex("edge_source_foreign_key_index", "edge(source_node_id)"));
  indices.emplace_back(STORAGE_MODE_CLEAR, SqliteDatabaseIndex("edge_target_foreign_key_index", "edge(target_node_id)"));
  indices.emplace_back(
      STORAGE_MODE_CLEAR, SqliteDatabaseIndex("source_location_foreign_key_index", "source_location(file_node_id)"));
  indices.emplace_back(STORAGE_MODE_CLEAR, SqliteDatabaseIndex("occurrence_element_foreign_key_index", "occurrence(element_id)"));
  indices.emplace_back(
      STORAGE_MODE_CLEAR, SqliteDatabaseIndex("occurrence_source_location_foreign_key_index", "occurrence(source_location_id)"));

  return indices;
}

void SqliteIndexStorage::clearTables() {
  try {
    m_database.execDML("DROP TABLE IF EXISTS main.error;");
    m_database.execDML("DROP TABLE IF EXISTS main.component_access;");
    m_database.execDML("DROP TABLE IF EXISTS main.occurrence;");
    m_database.execDML("DROP TABLE IF EXISTS main.source_location;");
    m_database.execDML("DROP TABLE IF EXISTS main.local_symbol;");
    m_database.execDML("DROP TABLE IF EXISTS main.filecontent;");
    m_database.execDML("DROP TABLE IF EXISTS main.file;");
    m_database.execDML("DROP TABLE IF EXISTS main.symbol;");
    m_database.execDML("DROP TABLE IF EXISTS main.node;");
    m_database.execDML("DROP TABLE IF EXISTS main.edge;");
    m_database.execDML("DROP TABLE IF EXISTS main.element_component;");
    m_database.execDML("DROP TABLE IF EXISTS main.element;");
    m_database.execDML("DROP TABLE IF EXISTS main.meta;");
  } catch(CppSQLite3Exception& e) {
    LOG_ERROR(std::to_string(e.errorCode()) + ": " + e.errorMessage());
  }
}

void SqliteIndexStorage::setupTables() {
  try {
    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS element("
        "id INTEGER, "
        "PRIMARY KEY(id));");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS element_component("
        "	id INTEGER, "
        "	element_id INTEGER, "
        "	type INTEGER, "
        "	data TEXT, "
        "	PRIMARY KEY(id), "
        "	FOREIGN KEY(element_id) REFERENCES element(id) ON DELETE CASCADE"
        ");");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS edge("
        "id INTEGER NOT NULL, "
        "type INTEGER NOT NULL, "
        "source_node_id INTEGER NOT NULL, "
        "target_node_id INTEGER NOT NULL, "
        "PRIMARY KEY(id), "
        "FOREIGN KEY(id) REFERENCES element(id) ON DELETE CASCADE, "
        "FOREIGN KEY(source_node_id) REFERENCES node(id) ON DELETE CASCADE, "
        "FOREIGN KEY(target_node_id) REFERENCES node(id) ON DELETE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS node("
        "id INTEGER NOT NULL, "
        "type INTEGER NOT NULL, "
        "serialized_name TEXT, "
        "PRIMARY KEY(id), "
        "FOREIGN KEY(id) REFERENCES element(id) ON DELETE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS symbol("
        "id INTEGER NOT NULL, "
        "definition_kind INTEGER NOT NULL, "
        "PRIMARY KEY(id), "
        "FOREIGN KEY(id) REFERENCES node(id) ON DELETE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS file("
        "id INTEGER NOT NULL, "
        "path TEXT, "
        "language TEXT, "
        "modification_time TEXT, "
        "indexed INTEGER, "
        "complete INTEGER, "
        "line_count INTEGER, "
        "PRIMARY KEY(id), "
        "FOREIGN KEY(id) REFERENCES node(id) ON DELETE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS filecontent("
        "id INTEGER, "
        "content TEXT, "
        "PRIMARY KEY(id), "
        "FOREIGN KEY(id) REFERENCES file(id)"
        "ON DELETE CASCADE "
        "ON UPDATE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS local_symbol("
        "id INTEGER NOT NULL, "
        "name TEXT, "
        "PRIMARY KEY(id), "
        "FOREIGN KEY(id) REFERENCES element(id) ON DELETE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS source_location("
        "id INTEGER NOT NULL, "
        "file_node_id INTEGER, "
        "start_line INTEGER, "
        "start_column INTEGER, "
        "end_line INTEGER, "
        "end_column INTEGER, "
        "type INTEGER, "
        "PRIMARY KEY(id), "
        "FOREIGN KEY(file_node_id) REFERENCES node(id) ON DELETE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS occurrence("
        "element_id INTEGER NOT NULL, "
        "source_location_id INTEGER NOT NULL, "
        "PRIMARY KEY(element_id, source_location_id), "
        "FOREIGN KEY(element_id) REFERENCES element(id) ON DELETE CASCADE, "
        "FOREIGN KEY(source_location_id) REFERENCES source_location(id) ON DELETE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS component_access("
        "node_id INTEGER NOT NULL, "
        "type INTEGER NOT NULL, "
        "PRIMARY KEY(node_id), "
        "FOREIGN KEY(node_id) REFERENCES node(id) ON DELETE CASCADE);");

    m_database.execDML(
        "CREATE TABLE IF NOT EXISTS error("
        "id INTEGER NOT NULL, "
        "message TEXT, "
        "fatal INTEGER NOT NULL, "
        "indexed INTEGER NOT NULL, "
        "translation_unit TEXT, "
        "PRIMARY KEY(id), "
        "FOREIGN KEY(id) REFERENCES element(id) ON DELETE CASCADE);");
  } catch(CppSQLite3Exception& e) {
    LOG_ERROR(std::to_string(e.errorCode()) + ": " + e.errorMessage());

    throw(std::exception());
  }
}

void SqliteIndexStorage::setupPrecompiledStatements() {
  try {
    m_insertNodeBatchStatement.compile(
        "INSERT INTO node(id, type, serialized_name) VALUES",
        3,
        [](CppSQLite3Statement& stmt, const StorageNode& node, size_t index) {
          stmt.bind(int(index) * 3 + 1, int(node.id));
          stmt.bind(int(index) * 3 + 2, int(node.type));
          stmt.bind(int(index) * 3 + 3, utility::encodeToUtf8(node.serializedName).c_str());
        },
        m_database);
    m_insertEdgeBatchStatement.compile(
        "INSERT INTO edge(id, type, source_node_id, target_node_id) VALUES",
        4,
        [](CppSQLite3Statement& stmt, const StorageEdge& edge, size_t index) {
          stmt.bind(int(index) * 4 + 1, int(edge.id));
          stmt.bind(int(index) * 4 + 2, int(edge.type));
          stmt.bind(int(index) * 4 + 3, int(edge.sourceNodeId));
          stmt.bind(int(index) * 4 + 4, int(edge.targetNodeId));
        },
        m_database);
    m_insertSymbolBatchStatement.compile(
        "INSERT OR IGNORE INTO symbol(id, definition_kind) VALUES",
        2,
        [](CppSQLite3Statement& stmt, const StorageSymbol& symbol, size_t index) {
          stmt.bind(int(index) * 2 + 1, int(symbol.id));
          stmt.bind(int(index) * 2 + 2, int(symbol.definitionKind));
        },
        m_database);
    m_insertLocalSymbolBatchStatement.compile(
        "INSERT INTO local_symbol(id, name) VALUES",
        2,
        [](CppSQLite3Statement& stmt, const StorageLocalSymbol& symbol, size_t index) {
          stmt.bind(int(index) * 2 + 1, int(symbol.id));
          stmt.bind(int(index) * 2 + 2, utility::encodeToUtf8(symbol.name).c_str());
        },
        m_database);
    m_insertSourceLocationBatchStatement.compile(
        "INSERT INTO source_location(file_node_id, start_line, start_column, end_line, "
        "end_column, type) VALUES",
        6,
        [](CppSQLite3Statement& stmt, const StorageSourceLocationData& location, size_t index) {
          stmt.bind(int(index) * 6 + 1, int(location.fileNodeId));
          stmt.bind(int(index) * 6 + 2, int(location.startLine));
          stmt.bind(int(index) * 6 + 3, int(location.startCol));
          stmt.bind(int(index) * 6 + 4, int(location.endLine));
          stmt.bind(int(index) * 6 + 5, int(location.endCol));
          stmt.bind(int(index) * 6 + 6, int(location.type));
        },
        m_database);
    m_insertOccurrenceBatchStatement.compile(
        "INSERT OR IGNORE INTO occurrence(element_id, source_location_id) VALUES",
        2,
        [](CppSQLite3Statement& stmt, const StorageOccurrence& occurrence, size_t index) {
          stmt.bind(int(index) * 2 + 1, int(occurrence.elementId));
          stmt.bind(int(index) * 2 + 2, int(occurrence.sourceLocationId));
        },
        m_database);
    m_insertComponentAccessBatchStatement.compile(
        "INSERT OR IGNORE INTO component_access(node_id, type) VALUES",
        2,
        [](CppSQLite3Statement& stmt, const StorageComponentAccess& componentAccess, size_t index) {
          stmt.bind(int(index) * 2 + 1, int(componentAccess.nodeId));
          stmt.bind(int(index) * 2 + 2, int(componentAccess.type));
        },
        m_database);

    m_insertElementStmt = m_database.compileStatement("INSERT INTO element(id) VALUES(NULL);");
    m_insertElementComponentStmt = m_database.compileStatement(
        "INSERT INTO element_component(id, element_id, type, data) VALUES(NULL, ?, ?, ?);");
    m_insertFileStmt = m_database.compileStatement(
        "INSERT INTO file(id, path, language, modification_time, indexed, complete, "
        "line_count) VALUES(?, ?, ?, ?, ?, ?, ?);");
    m_insertFileContentStmt = m_database.compileStatement("INSERT INTO filecontent(id, content) VALUES(?, ?);");
    m_checkErrorExistsStmt = m_database.compileStatement(
        "SELECT id FROM error WHERE "
        "message = ? AND "
        "fatal == ? "
        "LIMIT 1;");
    m_insertErrorStmt = m_database.compileStatement(
        "INSERT INTO error(id, message, fatal, indexed, translation_unit) "
        "VALUES(?, ?, ?, ?, ?);");
  } catch(CppSQLite3Exception& e) {
    LOG_ERROR(std::to_string(e.errorCode()) + ": " + e.errorMessage());

    throw(std::exception());

    // todo: cancel project creation and destroy created files, display message
  }
}

template <>
void SqliteIndexStorage::forEach<StorageEdge>(const std::string& query, const std::function<void(StorageEdge&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(fmt::format("SELECT id, type, source_node_id, target_node_id FROM edge {};", query));

  while(!queryResult.eof()) {
    const Id elementId = static_cast<Id>(queryResult.getIntField(0, 0));
    const int type = queryResult.getIntField(1, -1);
    const Id sourceId = static_cast<Id>(queryResult.getIntField(2, 0));
    const Id targetId = static_cast<Id>(queryResult.getIntField(3, 0));

    if(0 != elementId && -1 != type) {
      func(StorageEdge(elementId, type, sourceId, targetId));
    }

    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageNode>(const std::string& query, const std::function<void(StorageNode&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(fmt::format("SELECT id, type, serialized_name FROM node {};", query));

  while(!queryResult.eof()) {
    const Id elementId = static_cast<Id>(queryResult.getIntField(0, 0));
    const int type = queryResult.getIntField(1, -1);
    const std::string serializedName = queryResult.getStringField(2, "");

    if(0 != elementId && -1 != type) {
      func(StorageNode(elementId, type, utility::decodeFromUtf8(serializedName)));
    }

    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageSymbol>(const std::string& query, const std::function<void(StorageSymbol&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(fmt::format("SELECT id, definition_kind FROM symbol {};", query));

  while(!queryResult.eof()) {
    const Id elementId = static_cast<Id>(queryResult.getIntField(0, 0));
    const int definitionKind = queryResult.getIntField(1, 0);

    if(0 != elementId) {
      func(StorageSymbol(elementId, definitionKind));
    }

    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageFile>(const std::string& query, const std::function<void(StorageFile&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(
      fmt::format("SELECT id, path, language, modification_time, indexed, complete FROM file {};", query));

  while(!queryResult.eof()) {
    const Id elementId = static_cast<Id>(queryResult.getIntField(0, 0));
    const std::string filePath = queryResult.getStringField(1, "");
    const std::string languageIdentifier = queryResult.getStringField(2, "");
    const std::string modificationTime = queryResult.getStringField(3, "");
    const bool indexed = queryResult.getIntField(4, 0) != 0;
    const bool complete = queryResult.getIntField(5, 0) != 0;

    if(0 != elementId) {
      func(StorageFile(elementId,
                       utility::decodeFromUtf8(filePath),
                       utility::decodeFromUtf8(languageIdentifier),
                       modificationTime,
                       indexed,
                       complete));
    }
    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageLocalSymbol>(const std::string& query,
                                                     const std::function<void(StorageLocalSymbol&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(fmt::format("SELECT id, name FROM local_symbol {};", query));

  while(!queryResult.eof()) {
    const Id elementId = static_cast<Id>(queryResult.getIntField(0, 0));
    const std::string name = queryResult.getStringField(1, "");

    if(0 != elementId) {
      func(StorageLocalSymbol(elementId, utility::decodeFromUtf8(name)));
    }

    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageSourceLocation>(const std::string& query,
                                                        const std::function<void(StorageSourceLocation&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(
      fmt::format("SELECT id, file_node_id, start_line, start_column, end_line, end_column, type FROM "
                  "source_location {};",
                  query));

  while(!queryResult.eof()) {
    const Id elementId = static_cast<Id>(queryResult.getIntField(0, 0));
    const Id fileNodeId = static_cast<Id>(queryResult.getIntField(1, 0));
    const int startLineNumber = queryResult.getIntField(2, -1);
    const int startColNumber = queryResult.getIntField(3, -1);
    const int endLineNumber = queryResult.getIntField(4, -1);
    const int endColNumber = queryResult.getIntField(5, -1);
    const int type = queryResult.getIntField(6, -1);

    if(elementId != 0 && fileNodeId != 0 && startLineNumber != -1 && startColNumber != -1 && endLineNumber != -1 &&
       endColNumber != -1 && type != -1) {
      func(StorageSourceLocation(elementId,
                                 fileNodeId,
                                 static_cast<size_t>(startLineNumber),
                                 static_cast<size_t>(startColNumber),
                                 static_cast<size_t>(endLineNumber),
                                 static_cast<size_t>(endColNumber),
                                 type));
    }

    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageOccurrence>(const std::string& query,
                                                    const std::function<void(StorageOccurrence&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(fmt::format("SELECT element_id, source_location_id FROM occurrence {};", query));

  while(!queryResult.eof()) {
    const Id elementId = static_cast<Id>(queryResult.getIntField(0, 0));
    const Id sourceLocationId = static_cast<Id>(queryResult.getIntField(1, 0));

    if(0 != elementId && 0 != sourceLocationId) {
      func(StorageOccurrence(elementId, sourceLocationId));
    }

    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageComponentAccess>(const std::string& query,
                                                         const std::function<void(StorageComponentAccess&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(fmt::format("SELECT node_id, type FROM component_access {};", query));

  while(!queryResult.eof()) {
    const Id nodeId = static_cast<Id>(queryResult.getIntField(0, 0));
    const int type = queryResult.getIntField(1, -1);

    if(0 != nodeId && -1 != type) {
      func(StorageComponentAccess(nodeId, type));
    }

    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageElementComponent>(const std::string& query,
                                                          const std::function<void(StorageElementComponent&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(fmt::format("SELECT element_id, type, data FROM element_component {};", query));

  while(!queryResult.eof()) {
    const Id elementId = static_cast<uint32_t>(queryResult.getIntField(0, 0));
    const int type = queryResult.getIntField(1, -1);
    const std::string data = queryResult.getStringField(2, "");

    if(elementId != 0 && type != -1) {
      func(StorageElementComponent(elementId, type, utility::decodeFromUtf8(data)));
    }

    queryResult.nextRow();
  }
}

template <>
void SqliteIndexStorage::forEach<StorageError>(const std::string& query, const std::function<void(StorageError&&)>& func) const {
  CppSQLite3Query queryResult = executeQuery(
      fmt::format("SELECT id, message, fatal, indexed, translation_unit FROM error {};", query));

  while(!queryResult.eof()) {
    const Id errorId = static_cast<uint32_t>(queryResult.getIntField(0, 0));
    const std::string message = queryResult.getStringField(1, "");
    const bool fatal = queryResult.getIntField(2, 0) != 0;
    const bool indexed = queryResult.getIntField(3, 0) != 0;
    const std::string translationUnit = queryResult.getStringField(4, "");

    if(0 != errorId) {
      func(StorageError(errorId, utility::decodeFromUtf8(message), utility::decodeFromUtf8(translationUnit), fatal, indexed));
    }

    queryResult.nextRow();
  }
}
