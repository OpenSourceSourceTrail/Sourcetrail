#include "TestStorage.h"

#include "AccessKind.h"
#include "Edge.h"
#include "FilePath.h"
#include "LocationType.h"
#include "NameHierarchy.h"
#include "NodeType.h"
#include "Storage.h"
#include "utilityString.h"

std::shared_ptr<TestStorage> TestStorage::create(const std::shared_ptr<const Storage>& storage) {
  auto testStorage = std::make_shared<TestStorage>();

  std::map<Id, FilePath> filePathMap;
  for(const auto& file : storage->getStorageFiles()) {
    filePathMap.emplace(file.id, FilePath(file.filePath));
    testStorage->files.emplace(file.filePath);

    testStorage->addLine(L"FILE: " + FilePath(file.filePath).fileName() + (file.indexed ? L"" : L" non-indexed"));
  }

  std::map<Id, StorageComponentAccess> accessMap;
  for(const auto& access : storage->getComponentAccesses()) {
    accessMap.emplace(access.nodeId, access);
  }

  std::multimap<Id, Id> occurrenceMap;
  for(const auto& occurrence : storage->getStorageOccurrences()) {
    occurrenceMap.emplace(occurrence.sourceLocationId, occurrence.elementId);
  }

  std::multimap<Id, StorageSourceLocation> tokenLocationMap;
  std::multimap<Id, StorageSourceLocation> scopeLocationMap;
  std::multimap<Id, StorageSourceLocation> signatureLocationMap;
  std::multimap<Id, StorageSourceLocation> localSymbolLocationMap;
  std::multimap<Id, StorageSourceLocation> qualifierLocationMap;
  std::multimap<Id, StorageSourceLocation> errorLocationMap;
  std::vector<StorageSourceLocation> commentLocations;
  for(const auto& location : storage->getStorageSourceLocations()) {
    std::vector<Id> elementIds;
    for(auto it = occurrenceMap.find(location.id); it != occurrenceMap.end() && it->first == location.id; it++) {
      elementIds.emplace_back(it->second);
    }

    if(elementIds.empty()) {
      elementIds.emplace_back(0);
    }

    for(Id elementId : elementIds) {
      switch(intToLocationType(location.type)) {
      case LOCATION_TOKEN:
        if(elementId != 0U) {
          tokenLocationMap.emplace(elementId, location);
        }
        break;
      case LOCATION_SCOPE:
        if(elementId != 0U) {
          scopeLocationMap.emplace(elementId, location);
        }
        break;
      case LOCATION_QUALIFIER:
        if(elementId != 0U) {
          qualifierLocationMap.emplace(elementId, location);
        }
        break;
      case LOCATION_LOCAL_SYMBOL:
        if(elementId != 0U) {
          localSymbolLocationMap.emplace(elementId, location);
        }
        break;
      case LOCATION_SIGNATURE:
        if(elementId != 0U) {
          signatureLocationMap.emplace(elementId, location);
        }
        break;
      case LOCATION_ERROR:
        if(elementId != 0U) {
          errorLocationMap.emplace(elementId, location);
        }
        break;
      case LOCATION_COMMENT:
        commentLocations.emplace_back(location);
        break;
      default:
        break;
      }
    }
  }

  std::map<Id, StorageNode> nodesMap;
  std::set<Id> fileIdMap;
  for(const auto& node : storage->getStorageNodes()) {
    nodesMap.emplace(node.id, node);

    if(intToNodeKind(node.type) == NODE_FILE) {
      fileIdMap.insert(node.id);
    }

    std::wstring nameStr = NameHierarchy::deserialize(node.serializedName).getQualifiedNameWithSignature();

    for(auto qualifierLocationIt = qualifierLocationMap.find(node.id);
        qualifierLocationIt != qualifierLocationMap.end() && qualifierLocationIt->first == node.id;
        qualifierLocationIt++) {
      std::wstring locStr = addLocationStr(L"", qualifierLocationIt->second);
      testStorage->qualifiers.emplace_back(nameStr + locStr);
      testStorage->addLine(L"QUALIFIER: " + nameStr + addFileName(locStr, filePathMap[qualifierLocationIt->second.fileNodeId]));
    }

    auto* bin = testStorage->getBinForNodeType(node.type);
    if(bin != nullptr) {
      auto accessIt = accessMap.find(node.id);
      if(accessIt != accessMap.end()) {
        if(intToAccessKind(accessIt->second.type) != ACCESS_TEMPLATE_PARAMETER &&
           intToAccessKind(accessIt->second.type) != ACCESS_TYPE_PARAMETER) {
          nameStr = accessKindToString(intToAccessKind(accessIt->second.type)) + L' ' + nameStr;
        }
      }

      bool added = false;
      for(auto tokenLocationIt = tokenLocationMap.find(node.id);
          tokenLocationIt != tokenLocationMap.end() && tokenLocationIt->first == node.id;
          tokenLocationIt++) {
        added = false;
        std::wstring locationStr = addLocationStr(L"", tokenLocationIt->second);

        auto signatureLocationIt = signatureLocationMap.find(node.id);
        if(signatureLocationIt != signatureLocationMap.end() &&
           containsLocation(signatureLocationIt->second, tokenLocationIt->second)) {
          locationStr = addLocationStr(locationStr, signatureLocationIt->second);
        }

        for(auto scopeLocationIt = scopeLocationMap.find(node.id);
            scopeLocationIt != scopeLocationMap.end() && scopeLocationIt->first == node.id;
            scopeLocationIt++) {
          if(containsLocation(scopeLocationIt->second, tokenLocationIt->second)) {
            std::wstring locStr = addLocationStr(locationStr, scopeLocationIt->second);
            bin->emplace_back(nameStr + locStr);
            testStorage->addLine(nodeTypeToString(node.type) + L": " + nameStr +
                                 addFileName(locStr, filePathMap[tokenLocationIt->second.fileNodeId]));
            added = true;
          }
        }

        if(!added) {
          bin->emplace_back(nameStr + locationStr);
          testStorage->addLine(nodeTypeToString(node.type) + L": " + nameStr +
                               addFileName(locationStr, filePathMap[tokenLocationIt->second.fileNodeId]));
          added = true;
        }
      }

      if(!added) {
        bin->emplace_back(nameStr);
        testStorage->addLine(nodeTypeToString(node.type) + L": " + nameStr);
      }
    }
  }

  for(const auto& edge : storage->getStorageEdges()) {
    auto targetIt = nodesMap.find(edge.targetNodeId);
    if(targetIt == nodesMap.end()) {
      continue;
    }

    auto sourceIt = nodesMap.find(edge.sourceNodeId);
    if(sourceIt == nodesMap.end()) {
      continue;
    }

    const StorageNode& target = targetIt->second;
    const StorageNode& source = sourceIt->second;

    std::vector<std::wstring>* bin = testStorage->getBinForEdgeType(edge.type);
    if(bin != nullptr) {
      std::wstring sourceName = NameHierarchy::deserialize(source.serializedName).getQualifiedNameWithSignature();
      if(fileIdMap.find(edge.sourceNodeId) != fileIdMap.end() && FilePath(sourceName).exists()) {
        sourceName = FilePath(sourceName).fileName();
      }

      std::wstring targetName = NameHierarchy::deserialize(target.serializedName).getQualifiedNameWithSignature();
      if(fileIdMap.find(edge.targetNodeId) != fileIdMap.end() && FilePath(targetName).exists()) {
        targetName = FilePath(targetName).fileName();
      }

      std::wstring nameStr = sourceName + L" -> " + targetName;

      bool added = false;
      for(auto tokenLocationIt = tokenLocationMap.find(edge.id);
          tokenLocationIt != tokenLocationMap.end() && tokenLocationIt->first == edge.id;
          tokenLocationIt++) {
        std::wstring locStr = addLocationStr(L"", tokenLocationIt->second);
        bin->emplace_back(nameStr + locStr);
        testStorage->addLine(edgeTypeToString(edge.type) + L": " + nameStr +
                             addFileName(locStr, filePathMap[tokenLocationIt->second.fileNodeId]));
        added = true;
      }

      if(!added) {
        bin->emplace_back(nameStr);
        testStorage->addLine(edgeTypeToString(edge.type) + L": " + nameStr);
      }
    }
  }

  for(const auto& localSymbol : storage->getStorageLocalSymbols()) {
    bool added = false;
    for(auto localSymbolLocationIt = localSymbolLocationMap.find(localSymbol.id);
        localSymbolLocationIt != localSymbolLocationMap.end() && localSymbolLocationIt->first == localSymbol.id;
        localSymbolLocationIt++) {
      std::wstring locStr = addLocationStr(L"", localSymbolLocationIt->second);
      testStorage->localSymbols.emplace_back(localSymbol.name + locStr);
      testStorage->addLine(L"LOCAL_SYMBOL: " + localSymbol.name +
                           addFileName(locStr, filePathMap[localSymbolLocationIt->second.fileNodeId]));
      added = true;
    }

    if(!added) {
      testStorage->localSymbols.emplace_back(localSymbol.name);
      testStorage->addLine(L"LOCAL_SYMBOL: " + localSymbol.name);
    }
  }

  for(const auto& location : commentLocations) {
    std::wstring locStr = addLocationStr(L"", location);
    testStorage->comments.emplace_back(L"comment" + locStr);
    testStorage->addLine(L"COMMENT: comment" + addFileName(locStr, filePathMap[location.fileNodeId]));
  }

  for(const auto& error : storage->getErrors()) {
    for(auto errorLocationIt = errorLocationMap.find(error.id);
        errorLocationIt != errorLocationMap.end() && errorLocationIt->first == error.id;
        errorLocationIt++) {
      const auto& locStr = addLocationStr(L"", errorLocationIt->second);
      testStorage->errors.emplace_back(error.message + locStr);
      testStorage->addLine(L"ERROR: " + error.message + addFileName(locStr, filePathMap[errorLocationIt->second.fileNodeId]));
    }
  }

  return testStorage;
}

std::wstring TestStorage::nodeTypeToString(int nodeType) {
  switch(intToNodeKind(nodeType)) {
  case NODE_BUILTIN_TYPE:
    return L"SYMBOL_BUILTIN_TYPE";
  case NODE_CLASS:
    return L"SYMBOL_CLASS";
  case NODE_ENUM:
    return L"SYMBOL_ENUM";
  case NODE_ENUM_CONSTANT:
    return L"SYMBOL_ENUM_CONSTANT";
  case NODE_FIELD:
    return L"SYMBOL_FIELD";
  case NODE_FUNCTION:
    return L"SYMBOL_FUNCTION";
  case NODE_GLOBAL_VARIABLE:
    return L"SYMBOL_GLOBAL_VARIABLE";
  case NODE_INTERFACE:
    return L"SYMBOL_INTERFACE";
  case NODE_MACRO:
    return L"SYMBOL_MACRO";
  case NODE_METHOD:
    return L"SYMBOL_METHOD";
  case NODE_MODULE:
    return L"SYMBOL_MODULE";
  case NODE_NAMESPACE:
    return L"SYMBOL_NAMESPACE";
  case NODE_PACKAGE:
    return L"SYMBOL_PACKAGE";
  case NODE_STRUCT:
    return L"SYMBOL_STRUCT";
  case NODE_TYPEDEF:
    return L"SYMBOL_TYPEDEF";
  case NODE_TYPE_PARAMETER:
    return L"SYMBOL_TYPE_PARAMETER";
  case NODE_UNION:
    return L"SYMBOL_UNION";
  default:
    break;
  }
  return L"SYMBOL_NON_INDEXED";
}

std::wstring TestStorage::edgeTypeToString(int edgeType) {
  switch(Edge::intToType(edgeType)) {
  case Edge::EDGE_TYPE_USAGE:
    return L"REFERENCE_TYPE_USAGE";
  case Edge::EDGE_USAGE:
    return L"REFERENCE_USAGE";
  case Edge::EDGE_CALL:
    return L"REFERENCE_CALL";
  case Edge::EDGE_INHERITANCE:
    return L"REFERENCE_INHERITANCE";
  case Edge::EDGE_OVERRIDE:
    return L"REFERENCE_OVERRIDE";
  case Edge::EDGE_TYPE_ARGUMENT:
    return L"REFERENCE_TYPE_ARGUMENT";
  case Edge::EDGE_TEMPLATE_SPECIALIZATION:
    return L"REFERENCE_TEMPLATE_SPECIALIZATION";
  case Edge::EDGE_INCLUDE:
    return L"REFERENCE_INCLUDE";
  case Edge::EDGE_IMPORT:
    return L"REFERENCE_IMPORT";
  case Edge::EDGE_MACRO_USAGE:
    return L"REFERENCE_MACRO_USAGE";
  case Edge::EDGE_ANNOTATION_USAGE:
    return L"REFERENCE_ANNOTATION_USAGE";
  default:
    break;
  }
  return L"REFERENCE_UNDEFINED";
}

std::vector<std::wstring>* TestStorage::getBinForNodeType(int nodeType) {
  switch(intToNodeKind(nodeType)) {
  case NODE_PACKAGE:
    return &packages;
  case NODE_TYPEDEF:
    return &typedefs;
  case NODE_BUILTIN_TYPE:
    return &builtinTypes;
  case NODE_CLASS:
    return &classes;
  case NODE_UNION:
    return &unions;
  case NODE_INTERFACE:
    return &interfaces;
  case NODE_ANNOTATION:
    return &annotations;
  case NODE_ENUM:
    return &enums;
  case NODE_ENUM_CONSTANT:
    return &enumConstants;
  case NODE_FUNCTION:
    return &functions;
  case NODE_FIELD:
    return &fields;
  case NODE_GLOBAL_VARIABLE:
    return &globalVariables;
  case NODE_METHOD:
    return &methods;
  case NODE_MODULE:
    return &modules;
  case NODE_NAMESPACE:
    return &namespaces;
  case NODE_STRUCT:
    return &structs;
  case NODE_MACRO:
    return &macros;
  case NODE_TYPE_PARAMETER:
    return &typeParameters;
  default:
    break;
  }
  return nullptr;
}

std::vector<std::wstring>* TestStorage::getBinForEdgeType(int edgeType) {
  switch(Edge::intToType(edgeType)) {
  case Edge::EDGE_TYPE_USAGE:
    return &typeUses;
  case Edge::EDGE_USAGE:
    return &usages;
  case Edge::EDGE_CALL:
    return &calls;
  case Edge::EDGE_INHERITANCE:
    return &inheritances;
  case Edge::EDGE_OVERRIDE:
    return &overrides;
  case Edge::EDGE_TYPE_ARGUMENT:
    return &typeArguments;
  case Edge::EDGE_TEMPLATE_SPECIALIZATION:
    return &templateSpecializations;
  case Edge::EDGE_INCLUDE:
    return &includes;
  case Edge::EDGE_IMPORT:
    return &imports;
  case Edge::EDGE_MACRO_USAGE:
    return &macroUses;
  case Edge::EDGE_ANNOTATION_USAGE:
    return &annotationUses;
  default:
    break;
  }
  return nullptr;
}

std::wstring TestStorage::addLocationStr(const std::wstring& locationStr, const StorageSourceLocation& loc) {
  return L" <" + std::to_wstring(loc.startLine) + L':' + std::to_wstring(loc.startCol) + locationStr + L' ' +
      std::to_wstring(loc.endLine) + L':' + std::to_wstring(loc.endCol) + L'>';
}

std::wstring TestStorage::addFileName(const std::wstring& locationStr, const FilePath& filePath) {
  return L" [" + filePath.fileName() + locationStr + L']';
}

bool TestStorage::containsLocation(const StorageSourceLocation& outLocation, const StorageSourceLocation& inLocation) {
  if(outLocation.startLine > inLocation.startLine) {
    return false;
  }

  if(outLocation.startLine == inLocation.startLine && outLocation.startCol > inLocation.startCol) {
    return false;
  }

  if(outLocation.endLine < inLocation.endLine) {
    return false;
  }

  if(outLocation.endLine == inLocation.endLine && outLocation.endCol < inLocation.endCol) {
    return false;
  }

  return true;
}

void TestStorage::addLine(const std::wstring& message) {
  m_lines.emplace_back(utility::encodeToUtf8(message) + '\n');
}
