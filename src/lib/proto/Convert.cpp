#include "Convert.h"

#include "IndexerCommand.h"
#include "IntermediateStorage.h"
#include "StorageComponentAccess.h"
#include "StorageEdge.h"
#include "StorageError.h"
#include "StorageFile.h"
#include "StorageLocalSymbol.h"
#include "StorageNode.h"
#include "StorageOccurrence.h"
#include "StorageSourceLocation.h"
#include "StorageSymbol.h"
#include "language_packages.h"
#include "logging.h"
#include "utilityString.h"

#include "IndexerCommandJava.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "IndexerCommandCxx.h"
#endif

namespace proto::convert {

// ---- StorageNode -----------------------------------------------------------

sourcetrail::StorageNode toProto(const StorageNode& node) {
  sourcetrail::StorageNode msg;
  msg.set_id(node.id);
  msg.set_type(node.type);
  msg.set_serialized_name(utility::encodeToUtf8(node.serializedName));
  return msg;
}

StorageNode fromProto(const sourcetrail::StorageNode& msg) {
  return StorageNode(static_cast<Id>(msg.id()), msg.type(), utility::decodeFromUtf8(msg.serialized_name()));
}

// ---- StorageEdge -----------------------------------------------------------

sourcetrail::StorageEdge toProto(const StorageEdge& edge) {
  sourcetrail::StorageEdge msg;
  msg.set_id(edge.id);
  msg.set_type(edge.type);
  msg.set_source_node_id(edge.sourceNodeId);
  msg.set_target_node_id(edge.targetNodeId);
  return msg;
}

StorageEdge fromProto(const sourcetrail::StorageEdge& msg) {
  return StorageEdge(static_cast<Id>(msg.id()),
                     msg.type(),
                     static_cast<Id>(msg.source_node_id()),
                     static_cast<Id>(msg.target_node_id()));
}

// ---- StorageFile -----------------------------------------------------------

sourcetrail::StorageFile toProto(const StorageFile& file) {
  sourcetrail::StorageFile msg;
  msg.set_id(file.id);
  msg.set_file_path(utility::encodeToUtf8(file.filePath));
  msg.set_language_identifier(utility::encodeToUtf8(file.languageIdentifier));
  msg.set_modification_time(file.modificationTime);
  msg.set_indexed(file.indexed);
  msg.set_complete(file.complete);
  return msg;
}

StorageFile fromProto(const sourcetrail::StorageFile& msg) {
  return StorageFile(static_cast<Id>(msg.id()),
                     utility::decodeFromUtf8(msg.file_path()),
                     utility::decodeFromUtf8(msg.language_identifier()),
                     msg.modification_time(),
                     msg.indexed(),
                     msg.complete());
}

// ---- StorageSymbol ---------------------------------------------------------

sourcetrail::StorageSymbol toProto(const StorageSymbol& sym) {
  sourcetrail::StorageSymbol msg;
  msg.set_id(sym.id);
  msg.set_definition_kind(sym.definitionKind);
  return msg;
}

StorageSymbol fromProto(const sourcetrail::StorageSymbol& msg) {
  return StorageSymbol(static_cast<Id>(msg.id()), msg.definition_kind());
}

// ---- StorageLocalSymbol ----------------------------------------------------

sourcetrail::StorageLocalSymbol toProto(const StorageLocalSymbol& sym) {
  sourcetrail::StorageLocalSymbol msg;
  msg.set_id(sym.id);
  msg.set_name(utility::encodeToUtf8(sym.name));
  return msg;
}

StorageLocalSymbol fromProto(const sourcetrail::StorageLocalSymbol& msg) {
  return StorageLocalSymbol(static_cast<Id>(msg.id()), utility::decodeFromUtf8(msg.name()));
}

// ---- StorageSourceLocation -------------------------------------------------

sourcetrail::StorageSourceLocation toProto(const StorageSourceLocation& loc) {
  sourcetrail::StorageSourceLocation msg;
  msg.set_id(loc.id);
  msg.set_file_node_id(loc.fileNodeId);
  msg.set_start_line(static_cast<uint64_t>(loc.startLine));
  msg.set_start_col(static_cast<uint64_t>(loc.startCol));
  msg.set_end_line(static_cast<uint64_t>(loc.endLine));
  msg.set_end_col(static_cast<uint64_t>(loc.endCol));
  msg.set_type(loc.type);
  return msg;
}

StorageSourceLocation fromProto(const sourcetrail::StorageSourceLocation& msg) {
  return StorageSourceLocation(static_cast<Id>(msg.id()),
                               static_cast<Id>(msg.file_node_id()),
                               static_cast<size_t>(msg.start_line()),
                               static_cast<size_t>(msg.start_col()),
                               static_cast<size_t>(msg.end_line()),
                               static_cast<size_t>(msg.end_col()),
                               msg.type());
}

// ---- StorageOccurrence -----------------------------------------------------

sourcetrail::StorageOccurrence toProto(const StorageOccurrence& occ) {
  sourcetrail::StorageOccurrence msg;
  msg.set_element_id(occ.elementId);
  msg.set_source_location_id(occ.sourceLocationId);
  return msg;
}

StorageOccurrence fromProto(const sourcetrail::StorageOccurrence& msg) {
  return StorageOccurrence(static_cast<Id>(msg.element_id()), static_cast<Id>(msg.source_location_id()));
}

// ---- StorageComponentAccess ------------------------------------------------

sourcetrail::StorageComponentAccess toProto(const StorageComponentAccess& ca) {
  sourcetrail::StorageComponentAccess msg;
  msg.set_node_id(ca.nodeId);
  msg.set_type(ca.type);
  return msg;
}

StorageComponentAccess fromProto(const sourcetrail::StorageComponentAccess& msg) {
  return StorageComponentAccess(static_cast<Id>(msg.node_id()), msg.type());
}

// ---- StorageError ----------------------------------------------------------

sourcetrail::StorageError toProto(const StorageError& err) {
  sourcetrail::StorageError msg;
  msg.set_id(err.id);
  msg.set_message(utility::encodeToUtf8(err.message));
  msg.set_translation_unit(utility::encodeToUtf8(err.translationUnit));
  msg.set_fatal(err.fatal);
  msg.set_indexed(err.indexed);
  return msg;
}

StorageError fromProto(const sourcetrail::StorageError& msg) {
  return StorageError(static_cast<Id>(msg.id()),
                      utility::decodeFromUtf8(msg.message()),
                      utility::decodeFromUtf8(msg.translation_unit()),
                      msg.fatal(),
                      msg.indexed());
}

// ---- IndexerCommand (polymorphic) ------------------------------------------

sourcetrail::IndexerCommand toProto(const IndexerCommand* cmd) {
  sourcetrail::IndexerCommand msg;
  msg.set_source_file_path(utility::encodeToUtf8(cmd->getSourceFilePath().wstr()));

#if BUILD_CXX_LANGUAGE_PACKAGE
  if(const auto* cxx = dynamic_cast<const IndexerCommandCxx*>(cmd)) {
    msg.set_type(sourcetrail::IndexerCommand::CXX);

    for(const FilePath& p : cxx->getIndexedPaths()) {
      msg.add_indexed_paths(utility::encodeToUtf8(p.wstr()));
    }
    for(const FilePathFilter& f : cxx->getExcludeFilters()) {
      msg.add_exclude_filters(utility::encodeToUtf8(f.wstr()));
    }
    for(const FilePathFilter& f : cxx->getIncludeFilters()) {
      msg.add_include_filters(utility::encodeToUtf8(f.wstr()));
    }
    msg.set_working_directory(utility::encodeToUtf8(cxx->getWorkingDirectory().wstr()));
    for(const std::wstring& flag : cxx->getCompilerFlags()) {
      msg.add_compiler_flags(utility::encodeToUtf8(flag));
    }
    return msg;
  }
#endif

  if(const auto* java = dynamic_cast<const IndexerCommandJava*>(cmd)) {
    msg.set_type(sourcetrail::IndexerCommand::JAVA);

    for(const FilePath& p : java->getClassPaths()) {
      msg.add_class_paths(utility::encodeToUtf8(p.wstr()));
    }
    msg.set_language_standard(utility::encodeToUtf8(java->getLanguageStandard()));
    return msg;
  }

  LOG_ERROR(L"toProto: unhandled IndexerCommand type for file: " + cmd->getSourceFilePath().wstr());
  msg.set_type(sourcetrail::IndexerCommand::UNKNOWN);
  return msg;
}

std::shared_ptr<IndexerCommand> fromProto(const sourcetrail::IndexerCommand& msg) {
  switch(msg.type()) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  case sourcetrail::IndexerCommand::CXX: {
    FilePath sourceFilePath(utility::decodeFromUtf8(msg.source_file_path()));

    std::set<FilePath> indexedPaths;
    for(const auto& p : msg.indexed_paths()) {
      indexedPaths.insert(FilePath(utility::decodeFromUtf8(p)));
    }

    std::set<FilePathFilter> excludeFilters;
    for(const auto& f : msg.exclude_filters()) {
      excludeFilters.insert(FilePathFilter(utility::decodeFromUtf8(f)));
    }

    std::set<FilePathFilter> includeFilters;
    for(const auto& f : msg.include_filters()) {
      includeFilters.insert(FilePathFilter(utility::decodeFromUtf8(f)));
    }

    FilePath workingDir(utility::decodeFromUtf8(msg.working_directory()));

    std::vector<std::wstring> compilerFlags;
    for(const auto& flag : msg.compiler_flags()) {
      compilerFlags.push_back(utility::decodeFromUtf8(flag));
    }

    return std::make_shared<IndexerCommandCxx>(
        sourceFilePath, indexedPaths, excludeFilters, includeFilters, workingDir, compilerFlags);
  }
#endif

  case sourcetrail::IndexerCommand::JAVA: {
    FilePath sourceFilePath(utility::decodeFromUtf8(msg.source_file_path()));

    std::set<FilePath> classPaths;
    for(const auto& p : msg.class_paths()) {
      classPaths.insert(FilePath(utility::decodeFromUtf8(p)));
    }

    std::wstring languageStandard = utility::decodeFromUtf8(msg.language_standard());

    return std::make_shared<IndexerCommandJava>(sourceFilePath, classPaths, languageStandard);
  }

  case sourcetrail::IndexerCommand::UNKNOWN:
  default:
    LOG_ERROR("fromProto: unhandled IndexerCommand type: " + std::to_string(static_cast<int>(msg.type())));
    return nullptr;
  }
}

// ---- IntermediateStorage ---------------------------------------------------

sourcetrail::IntermediateStorage toProto(const IntermediateStorage& storage) {
  sourcetrail::IntermediateStorage msg;

  for(const StorageNode& n : storage.getStorageNodes()) {
    *msg.add_nodes() = toProto(n);
  }
  for(const StorageFile& f : storage.getStorageFiles()) {
    *msg.add_files() = toProto(f);
  }
  for(const StorageSymbol& s : storage.getStorageSymbols()) {
    *msg.add_symbols() = toProto(s);
  }
  for(const StorageEdge& e : storage.getStorageEdges()) {
    *msg.add_edges() = toProto(e);
  }
  for(const StorageLocalSymbol& l : storage.getStorageLocalSymbols()) {
    *msg.add_local_symbols() = toProto(l);
  }
  for(const StorageSourceLocation& loc : storage.getStorageSourceLocations()) {
    *msg.add_source_locations() = toProto(loc);
  }
  for(const StorageOccurrence& o : storage.getStorageOccurrences()) {
    *msg.add_occurrences() = toProto(o);
  }
  for(const StorageComponentAccess& ca : storage.getComponentAccesses()) {
    *msg.add_component_accesses() = toProto(ca);
  }
  for(const StorageError& err : storage.getErrors()) {
    *msg.add_errors() = toProto(err);
  }
  msg.set_next_id(storage.getNextId());

  return msg;
}

std::shared_ptr<IntermediateStorage> fromProto(const sourcetrail::IntermediateStorage& msg) {
  auto storage = std::make_shared<IntermediateStorage>();

  std::vector<StorageNode> nodes;
  nodes.reserve(static_cast<size_t>(msg.nodes_size()));
  for(const auto& n : msg.nodes()) {
    nodes.push_back(fromProto(n));
  }
  storage->setStorageNodes(std::move(nodes));

  std::vector<StorageFile> files;
  files.reserve(static_cast<size_t>(msg.files_size()));
  for(const auto& f : msg.files()) {
    files.push_back(fromProto(f));
  }
  storage->setStorageFiles(std::move(files));

  std::vector<StorageSymbol> symbols;
  symbols.reserve(static_cast<size_t>(msg.symbols_size()));
  for(const auto& s : msg.symbols()) {
    symbols.push_back(fromProto(s));
  }
  storage->setStorageSymbols(std::move(symbols));

  std::vector<StorageEdge> edges;
  edges.reserve(static_cast<size_t>(msg.edges_size()));
  for(const auto& e : msg.edges()) {
    edges.push_back(fromProto(e));
  }
  storage->setStorageEdges(std::move(edges));

  std::set<StorageLocalSymbol> localSymbols;
  for(const auto& l : msg.local_symbols()) {
    localSymbols.insert(fromProto(l));
  }
  storage->setStorageLocalSymbols(std::move(localSymbols));

  std::set<StorageSourceLocation> sourceLocs;
  for(const auto& loc : msg.source_locations()) {
    sourceLocs.insert(fromProto(loc));
  }
  storage->setStorageSourceLocations(std::move(sourceLocs));

  std::set<StorageOccurrence> occurrences;
  for(const auto& o : msg.occurrences()) {
    occurrences.insert(fromProto(o));
  }
  storage->setStorageOccurrences(std::move(occurrences));

  std::set<StorageComponentAccess> componentAccesses;
  for(const auto& ca : msg.component_accesses()) {
    componentAccesses.insert(fromProto(ca));
  }
  storage->setComponentAccesses(std::move(componentAccesses));

  std::vector<StorageError> errors;
  errors.reserve(static_cast<size_t>(msg.errors_size()));
  for(const auto& err : msg.errors()) {
    errors.push_back(fromProto(err));
  }
  storage->setErrors(std::move(errors));

  storage->setNextId(static_cast<Id>(msg.next_id()));

  return storage;
}

}    // namespace proto::convert
