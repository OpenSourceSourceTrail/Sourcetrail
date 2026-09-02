#include "EngineHttpService.h"

#include <algorithm>
#include <string>
#include <vector>

#include "app/Application.h"
#include "app/IndexerPluginRegistry.h"
#include "ConvertGraph.h"
#include "ConvertLocations.h"
#include "ConvertQuery.h"
#include "bookmark/domain/BookmarkCategory.h"
#include "bookmark/domain/EdgeBookmark.h"
#include "bookmark/domain/NodeBookmark.h"
#include "data/ErrorFilter.h"
#include "data/ErrorInfo.h"
#include "data/graph/Graph.h"
#include "data/location/SourceLocationCollection.h"
#include "data/location/SourceLocationFile.h"
#include "data/name/NameHierarchy.h"
#include "data/NodeKind.h"
#include "data/NodeType.h"
#include "data/NodeTypeSet.h"
#include "data/search/SearchMatch.h"
#include "data/storage/StorageAccess.h"
#include "data/tooltip/TooltipInfo.h"
#include "engine.pb.h"
#include "FileInfo.h"
#include "FilePath.h"
#include "logging.h"
#include "project/CompilationDatabase.h"
#include "project/utilitySourceGroupCxx.h"
#include "ProtoJson.h"
#include "TextAccess.h"
#include "TimeStamp.h"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "type/MessageRefresh.h"
#include "utilityString.h"
#include "Version.h"

namespace {

http::Response ok(const google::protobuf::Message& message) {
  return http::Response::json(proto::json::toJson(message));
}

http::Response okEmpty() {
  return http::Response::json("{}");
}

http::Response badRequest(std::string_view what) {
  return http::Response::error(400, what);
}

std::string toUtf8(const std::wstring& text) {
  return utility::encodeToUtf8(text);
}

std::wstring fromUtf8(const std::string& text) {
  return utility::decodeFromUtf8(text);
}

/** Parses a comma-separated list of ids from a query parameter. */
std::vector<Id> idList(const http::Request& request, const std::string& key) {
  std::vector<Id> ids;
  for(const std::string& part : request.getList(key)) {
    try {
      ids.push_back(static_cast<Id>(std::stoull(part)));
    } catch(...) {
      // A malformed id is dropped rather than failing the whole request: the caller asked about a
      // set, and the rest of the set is still answerable.
      LOG_WARNING("Ignoring malformed id '" + part + "' in query parameter '" + key + "'");
    }
  }
  return ids;
}

bool wants(const http::Request& request, std::string_view part) {
  const std::vector<std::string> include = request.getList("include");
  // No `include` at all means "everything this endpoint offers"; naming one narrows it.
  if(include.empty()) {
    return true;
  }
  return std::find(include.begin(), include.end(), part) != include.end();
}

ErrorFilter errorFilterFrom(const http::Request& request) {
  sourcetrail::ProtoErrorFilter filter;
  filter.set_error(request.getBool("error", true));
  filter.set_fatal(request.getBool("fatal", true));
  filter.set_unindexed_error(request.getBool("unindexedError", true));
  filter.set_unindexed_fatal(request.getBool("unindexedFatal", true));
  filter.set_limit(request.getUInt("limit", 0));
  return proto::convert::fromProto(filter);
}

template <class Message>
bool parseBody(const http::Request& request, Message& message) {
  return proto::json::fromJson(request.body, message);
}

}    // namespace

EngineHttpService::EngineHttpService(StorageAccess* storageAccess) : mStorageAccess(storageAccess), mGraphLayout(storageAccess) {}

EngineHttpService::~EngineHttpService() {
  abortDialogs();
}

void EngineHttpService::setShutdownHandler(std::function<void()> handler) {
  mShutdownHandler = std::move(handler);
}

// ---- Event stream and dialogs ----------------------------------------------

void EngineHttpService::broadcastEvent(const sourcetrail::EngineEvent& event) {
  if(mServer == nullptr) {
    return;
  }

  // The SSE event name is the oneof arm, so a browser can addEventListener per event type instead of
  // switching on a discriminator inside the payload.
  const char* name = "unknown";
  switch(event.event_case()) {
  case sourcetrail::EngineEvent::kIndexingStarted:
    name = "indexingStarted";
    break;
  case sourcetrail::EngineEvent::kIndexingProgress:
    name = "indexingProgress";
    break;
  case sourcetrail::EngineEvent::kIndexingFinished:
    name = "indexingFinished";
    break;
  case sourcetrail::EngineEvent::kIndexingError:
    name = "indexingError";
    break;
  case sourcetrail::EngineEvent::kStatusInfo:
    name = "statusInfo";
    break;
  case sourcetrail::EngineEvent::kUnknownProgress:
    name = "unknownProgress";
    break;
  case sourcetrail::EngineEvent::kProgress:
    name = "progress";
    break;
  case sourcetrail::EngineEvent::kClearDialogs:
    name = "clearDialogs";
    break;
  case sourcetrail::EngineEvent::kErrorCount:
    name = "errorCount";
    break;
  case sourcetrail::EngineEvent::EVENT_NOT_SET:
  default:
    LOG_WARNING("Refusing to broadcast an engine event with no payload.");
    return;
  }

  mServer->broadcast(name, proto::json::toJson(event));
}

std::optional<int> EngineHttpService::askDialog(const sourcetrail::DialogRequest& request, std::chrono::milliseconds timeout) {
  if(mServer == nullptr) {
    return std::nullopt;
  }

  uint64_t requestId = 0;
  {
    const std::lock_guard<std::mutex> lock(mDialogMutex);
    if(mDialogsAborted) {
      return std::nullopt;
    }
    requestId = mNextDialogId++;
    mPendingDialogs[requestId];
  }

  sourcetrail::DialogRequest outgoing = request;
  outgoing.set_request_id(requestId);
  mServer->broadcast("dialog", proto::json::toJson(outgoing));

  std::unique_lock<std::mutex> lock(mDialogMutex);
  const bool answered = mDialogSignal.wait_for(lock, timeout, [&] {
    const auto found = mPendingDialogs.find(requestId);
    return mDialogsAborted || found == mPendingDialogs.end() || found->second.answer.has_value();
  });

  std::optional<int> answer;
  if(const auto found = mPendingDialogs.find(requestId); found != mPendingDialogs.end()) {
    if(answered) {
      answer = found->second.answer;
    }
    mPendingDialogs.erase(found);
  }
  return answer;
}

void EngineHttpService::abortDialogs() {
  {
    const std::lock_guard<std::mutex> lock(mDialogMutex);
    mDialogsAborted = true;
    mPendingDialogs.clear();
  }
  mDialogSignal.notify_all();
}

http::Response EngineHttpService::handleDialogResponse(const http::Request& request) {
  sourcetrail::DialogResponse response;
  if(!parseBody(request, response)) {
    return badRequest("Malformed dialog response");
  }

  uint64_t requestId = response.request_id();
  if(requestId == 0) {
    try {
      requestId = std::stoull(request.param);
    } catch(...) {
      return badRequest("Missing dialog request id");
    }
  }

  {
    const std::lock_guard<std::mutex> lock(mDialogMutex);
    const auto found = mPendingDialogs.find(requestId);
    if(found == mPendingDialogs.end()) {
      // The asking thread gave up, or this id was never handed out.
      return http::Response::error(404, "No dialog is waiting on that id");
    }
    found->second.answer = response.selected_option();
  }
  mDialogSignal.notify_all();
  return okEmpty();
}

// ---- Routes ----------------------------------------------------------------

// NOLINTNEXTLINE(readability-function-size)
void EngineHttpService::registerRoutes(http::Server& server) {
  mServer = &server;
  server.eventStream("/api/v1/events");

  // -- Lifecycle -------------------------------------------------------------

  server.route("GET", "/api/v1/capabilities", [](const http::Request&) {
    const IndexerPluginRegistry::Ptr registry = IndexerPluginRegistry::getInstance();

    sourcetrail::CapabilitiesResponse response;
    for(const IndexerPluginRegistry::Plugin& plugin : registry->getPlugins()) {
      sourcetrail::PluginInfo* info = response.add_plugins();
      info->set_id(plugin.id);
      info->set_name(plugin.name);
      info->set_language(plugin.language);
      info->set_command_type(plugin.commandType);
      for(const SourceGroupType type : plugin.sourceGroupTypes) {
        info->add_source_group_types(type);
      }
      response.add_supported_languages(plugin.language);
    }
    for(const SourceGroupType type : registry->availableSourceGroupTypes()) {
      response.add_supported_source_group_types(type);
    }

    // Custom Command needs no plugin at all -- the user supplies the command line -- so a bare
    // install can still create a project. The client goes read-only when the engine is
    // *unreachable*, not when it is empty.
    response.add_supported_source_group_types(SOURCE_GROUP_CUSTOM_COMMAND);
    response.add_supported_languages(LANGUAGE_CUSTOM);
    response.set_can_create_project(response.supported_source_group_types_size() > 0);
    response.set_engine_version(Version::getApplicationVersion().toString());
    return ok(response);
  });

  const auto projectState = []() {
    sourcetrail::LoadProjectResponse response;
    const std::shared_ptr<IProject> project = Application::getInstance()->getCurrentProject();
    if(!project) {
      response.set_error_message("No project is loaded.");
      return response;
    }
    response.set_loaded(project->isLoaded());
    response.set_indexing(project->isIndexing());
    response.set_reindexable(project->isReindexable());
    response.set_description(project->getDescription());
    return response;
  };

  server.route("GET", "/api/v1/project", [projectState](const http::Request&) { return ok(projectState()); });

  server.route("PUT", "/api/v1/project", [projectState](const http::Request& request) {
    sourcetrail::LoadProjectRequest body;
    if(!parseBody(request, body) || body.project_file_path().empty()) {
      return badRequest("Missing project_file_path");
    }

    // Synchronous on purpose: the client is blocked on this response and has no other way to learn
    // whether the project opened. setSendAsTask(false) is what makes dispatchImmediately immediate --
    // MessageQueue::processMessage still hands the message to the task queue otherwise.
    MessageLoadProject message{FilePath(fromUtf8(body.project_file_path())), false};
    message.setSendAsTask(false);
    message.dispatchImmediately();

    return ok(projectState());
  });

  server.route("POST", "/api/v1/project/refresh", [](const http::Request& request) {
    sourcetrail::RefreshRequest body;
    if(!parseBody(request, body)) {
      return badRequest("Malformed refresh request");
    }
    MessageRefresh message;
    if(body.all()) {
      message.refreshAll();
    }
    message.dispatch();
    return okEmpty();
  });

  // Previously declared but never implemented, so every client call logged a warning and marked the
  // connection degraded. The state change is what tells the client the index is stale.
  server.route("POST", "/api/v1/project/state/outdated", [](const http::Request&) {
    const std::shared_ptr<IProject> project = Application::getInstance()->getCurrentProject();
    if(!project) {
      return http::Response::error(409, "No project is loaded");
    }
    project->setStateOutdated();
    return okEmpty();
  });

  // The GUI stop button used to dispatch a message that never left the client process, so cancelling
  // an index could not reach the engine at all.
  server.route("POST", "/api/v1/project/indexing/cancel", [](const http::Request&) {
    MessageIndexingInterrupted{}.dispatch();
    return okEmpty();
  });

  server.route("POST", "/api/v1/shutdown", [this](const http::Request&) {
    if(mShutdownHandler) {
      mShutdownHandler();
    }
    return okEmpty();
  });

  server.route("POST", "/api/v1/compilation-database", [](const http::Request& request) {
    sourcetrail::CompilationDatabaseInfoRequest body;
    if(!parseBody(request, body)) {
      return badRequest("Malformed compilation database request");
    }

    sourcetrail::CompilationDatabaseInfoResponse response;
    const FilePath cdbPath(fromUtf8(body.cdb_path()));
    if(cdbPath.empty() || !cdbPath.exists()) {
      response.set_error("The provided Compilation Database path does not exist.");
      return ok(response);
    }

    std::string error;
    const std::optional<std::vector<CxxCompileCommand>> commands = utility::loadCompilationDatabase(cdbPath, &error);
    if(!commands) {
      response.set_error(error.empty() ? "Unable to open and read the provided compilation database file." : error);
      return ok(response);
    }

    response.set_valid(true);
    for(const FilePath& path : utility::getSourceFilesFromCDB(*commands, cdbPath)) {
      response.add_source_files(toUtf8(path.wstr()));
    }
    for(const FilePath& path : utility::CompilationDatabase(*commands).getAllHeaderPaths()) {
      response.add_header_paths(toUtf8(path.wstr()));
    }
    response.set_contains_include_pch_flags(utility::containsIncludePchFlags(*commands));
    return ok(response);
  });

  server.route("POST", "/api/v1/dialogs/", [this](const http::Request& request) { return handleDialogResponse(request); });

  // -- Index summary ---------------------------------------------------------

  // Merged because the start screen and status bar fetch all of this together. `include` exists so
  // the callers that want one part -- the status bar asks for error counts on every refresh -- do not
  // make the engine run all four queries.
  server.route("GET", "/api/v1/stats", [this](const http::Request& request) {
    sourcetrail::StatsResponse response;

    if(wants(request, "counts")) {
      const StorageStats stats = mStorageAccess->getStorageStats();
      auto* protoStats = response.mutable_stats();
      protoStats->set_node_count(stats.nodeCount);
      protoStats->set_edge_count(stats.edgeCount);
      protoStats->set_file_count(stats.fileCount);
      protoStats->set_completed_file_count(stats.completedFileCount);
      protoStats->set_file_loc_count(stats.fileLOCCount);
    }

    if(wants(request, "errors")) {
      const ErrorCountInfo errors = mStorageAccess->getErrorCount();
      response.set_error_total(errors.total);
      response.set_error_fatal(errors.fatal);
    }

    if(wants(request, "types")) {
      response.set_available_node_types(static_cast<int32_t>(mStorageAccess->getAvailableNodeTypes()));
      response.set_available_edge_types(static_cast<int32_t>(mStorageAccess->getAvailableEdgeTypes()));
    }
    return ok(response);
  });

  // -- Graph -----------------------------------------------------------------

  server.route("GET", "/api/v1/graph", [this](const http::Request& request) {
    const std::string mode = request.get("mode", "all");

    sourcetrail::GraphQueryResponse response;
    std::shared_ptr<Graph> graph;

    if(mode == "all") {
      graph = mStorageAccess->getGraphForAll();
    } else if(mode == "types") {
      graph = mStorageAccess->getGraphForNodeTypes(
          proto::convert::nodeTypeSetFromMask(static_cast<int>(request.getUInt("mask"))));
    } else if(mode == "active") {
      bool isActiveNamespace = false;
      graph = mStorageAccess->getGraphForActiveTokenIds(idList(request, "tokens"), idList(request, "expanded"), &isActiveNamespace);
      response.set_is_active_namespace(isActiveNamespace);
    } else if(mode == "children") {
      graph = mStorageAccess->getGraphForChildrenOfNodeId(static_cast<Id>(request.getUInt("id")));
    } else if(mode == "trail") {
      graph = mStorageAccess->getGraphForTrail(static_cast<Id>(request.getUInt("origin")),
                                               static_cast<Id>(request.getUInt("target")),
                                               static_cast<int>(request.getUInt("nodeMask")),
                                               static_cast<int>(request.getUInt("edgeMask")),
                                               request.getBool("nonIndexed"),
                                               request.getUInt("depth"),
                                               request.getBool("directed"));
    } else {
      return badRequest("Unknown graph mode '" + mode + "'");
    }

    if(graph) {
      *response.mutable_graph() = proto::convert::toProto(*graph);
    }

    // Folded in so graph layout does not need a second round trip to group nodes by file.
    if(graph && request.getBool("parentFiles")) {
      std::vector<Id> nodeIds;
      graph->forEachNode([&nodeIds](Node* node) { nodeIds.push_back(node->getId()); });
      for(const auto& [nodeId, parent] : mStorageAccess->getNodeIdToParentFileMap(nodeIds)) {
        auto* entry = response.add_parent_files();
        entry->set_node_id(nodeId);
        entry->set_parent_file_id(parent.first);
        entry->set_name_hierarchy_serialized(toUtf8(NameHierarchy::serialize(parent.second)));
      }
    }
    return ok(response);
  });

  // -- Batch symbol resolution ----------------------------------------------

  // Same graph data as GET /api/v1/graph, but run through the layout the Qt view drives, so a
  // client that cannot link BucketLayouter gets positions instead of having to reinvent them.
  // POST rather than GET: the client's font metrics are part of the input.
  server.route("POST", "/api/v1/graph/layout", [this](const http::Request& request) {
    sourcetrail::GraphLayoutRequest body;
    if(!parseBody(request, body)) {
      return badRequest("Malformed layout request");
    }
    if(body.char_width() <= 0.0F || body.char_height() <= 0.0F) {
      return badRequest("char_width and char_height are required: node boxes are sized from them");
    }
    return ok(mGraphLayout.layout(body));
  });

  server.route("POST", "/api/v1/symbols/resolve", [this](const http::Request& request) {
    sourcetrail::SymbolResolveRequest body;
    if(!parseBody(request, body)) {
      return badRequest("Malformed resolve request");
    }

    sourcetrail::SymbolResolveResponse response;

    for(const uint64_t nodeId : body.node_ids()) {
      auto* resolved = response.add_nodes();
      resolved->set_id(nodeId);
      resolved->set_name_hierarchy_serialized(
          toUtf8(NameHierarchy::serialize(mStorageAccess->getNameHierarchyForNodeId(static_cast<Id>(nodeId)))));
      // Only when asked: callers that want a display name pay one lookup, not two.
      if(body.include_node_kinds()) {
        resolved->set_node_kind(nodeKindToInt(mStorageAccess->getNodeTypeForNodeWithId(static_cast<Id>(nodeId)).getKind()));
      }
    }

    for(const uint64_t edgeId : body.edge_ids()) {
      const StorageEdge edge = mStorageAccess->getEdgeById(static_cast<Id>(edgeId));
      auto* protoEdge = response.add_edges();
      protoEdge->set_id(edge.id);
      protoEdge->set_type(edge.type);
      protoEdge->set_source_node_id(edge.sourceNodeId);
      protoEdge->set_target_node_id(edge.targetNodeId);
    }

    for(const std::string& serialized : body.name_hierarchies()) {
      response.add_name_hierarchy_node_ids(
          mStorageAccess->getNodeIdForNameHierarchy(NameHierarchy::deserialize(fromUtf8(serialized))));
    }

    for(const std::string& path : body.file_paths()) {
      response.add_file_path_node_ids(mStorageAccess->getNodeIdForFileNode(FilePath(fromUtf8(path))));
    }

    if(body.location_ids_size() > 0) {
      const std::vector<Id> locationIds(body.location_ids().begin(), body.location_ids().end());
      for(const Id id : mStorageAccess->getNodeIdsForLocationIds(locationIds)) {
        response.add_location_node_ids(id);
      }
    }

    if(body.search_match_token_ids_size() > 0) {
      const std::vector<Id> tokenIds(body.search_match_token_ids().begin(), body.search_match_token_ids().end());
      for(const SearchMatch& match : mStorageAccess->getSearchMatchesForTokenIds(tokenIds)) {
        *response.add_search_matches() = proto::convert::toProto(match);
      }
    }

    if(body.parent_file_node_ids_size() > 0) {
      const std::vector<Id> nodeIds(body.parent_file_node_ids().begin(), body.parent_file_node_ids().end());
      for(const auto& [nodeId, parent] : mStorageAccess->getNodeIdToParentFileMap(nodeIds)) {
        auto* entry = response.add_parent_files();
        entry->set_node_id(nodeId);
        entry->set_parent_file_id(parent.first);
        entry->set_name_hierarchy_serialized(toUtf8(NameHierarchy::serialize(parent.second)));
      }
    }
    return ok(response);
  });

  server.route("GET", "/api/v1/tokens/active", [this](const http::Request& request) {
    Id declarationId = 0;
    const std::vector<Id> ids = mStorageAccess->getActiveTokenIdsForId(static_cast<Id>(request.getUInt("id")), &declarationId);

    sourcetrail::ActiveTokenIdsResponse response;
    for(const Id id : ids) {
      response.add_ids(id);
    }
    response.set_declaration_id(declarationId);
    return ok(response);
  });

  // -- Search ----------------------------------------------------------------

  server.route("GET", "/api/v1/search", [this](const http::Request& request) {
    sourcetrail::SearchMatchesResponse response;
    const auto matches = mStorageAccess->getAutocompletionMatches(
        fromUtf8(request.get("q")),
        proto::convert::nodeTypeSetFromMask(static_cast<int>(request.getUInt("types"))),
        request.getBool("commands"));
    for(const SearchMatch& match : matches) {
      *response.add_matches() = proto::convert::toProto(match);
    }
    return ok(response);
  });

  server.route("GET", "/api/v1/search/fulltext", [this](const http::Request& request) {
    const auto collection = mStorageAccess->getFullTextSearchLocations(fromUtf8(request.get("q")), request.getBool("case"));
    return ok(collection ? proto::convert::toProto(*collection) : sourcetrail::SourceLocationCollectionResponse{});
  });

  // -- Files and locations ---------------------------------------------------

  // One response where opening a file used to cost four round trips. `include` narrows it, `lines`
  // and `locationType` narrow the location set the same way the former dedicated RPCs did.
  server.route("GET", "/api/v1/files/", [this](const http::Request& request) {
    if(request.param.empty()) {
      return badRequest("Missing file path");
    }
    const FilePath filePath(fromUtf8(request.param));

    sourcetrail::FileResponse response;

    if(wants(request, "content")) {
      if(const auto text = mStorageAccess->getFileContent(filePath, request.getBool("showsErrors")); text) {
        response.set_content(text->getText());
      }
    }

    if(wants(request, "locations")) {
      std::shared_ptr<SourceLocationFile> locations;
      if(request.query.contains("lines")) {
        const std::vector<std::string> range = request.getList("lines");
        uint64_t startLine = 0;
        uint64_t endLine = 0;
        try {
          startLine = range.empty() ? 0 : std::stoull(range.front());
          endLine = range.size() > 1 ? std::stoull(range[1]) : startLine;
        } catch(...) {
          // Every other malformed parameter on this route answers 400; an unguarded stoull here
          // would escape the handler and surface as a 500 instead.
          return badRequest("Malformed line range");
        }
        locations = mStorageAccess->getSourceLocationsForLinesInFile(filePath, startLine, endLine);
      } else if(request.query.contains("locationType")) {
        locations = mStorageAccess->getSourceLocationsOfTypeInFile(
            filePath, static_cast<LocationType>(request.getUInt("locationType")));
      } else {
        locations = mStorageAccess->getSourceLocationsForFile(filePath);
      }

      if(locations) {
        *response.mutable_locations() = proto::convert::toProto(*locations);
      } else {
        // The code view titles itself from this path, so an unindexed file still answers with one.
        response.mutable_locations()->set_file_path(request.param);
      }
    }

    if(wants(request, "info")) {
      *response.mutable_info() = proto::convert::toProto(mStorageAccess->getFileInfoForFilePath(filePath));
    }

    if(wants(request, "errors")) {
      for(const ErrorInfo& error : mStorageAccess->getErrorsForFileLimited(errorFilterFrom(request), filePath)) {
        *response.add_errors() = proto::convert::toProto(error);
      }
    }
    return ok(response);
  });

  server.route("POST", "/api/v1/files/info", [this](const http::Request& request) {
    sourcetrail::FilePathsRequest body;
    if(!parseBody(request, body)) {
      return badRequest("Malformed file info request");
    }

    std::vector<FilePath> filePaths;
    filePaths.reserve(static_cast<size_t>(body.file_paths_size()));
    for(const std::string& path : body.file_paths()) {
      filePaths.emplace_back(fromUtf8(path));
    }

    sourcetrail::FileInfosResponse response;
    for(const FileInfo& info : mStorageAccess->getFileInfosForFilePaths(filePaths)) {
      *response.add_file_infos() = proto::convert::toProto(info);
    }
    return ok(response);
  });

  server.route("GET", "/api/v1/locations", [this](const http::Request& request) {
    std::shared_ptr<SourceLocationCollection> collection;
    if(request.query.contains("tokens")) {
      collection = mStorageAccess->getSourceLocationsForTokenIds(idList(request, "tokens"));
    } else if(request.query.contains("locations")) {
      collection = mStorageAccess->getSourceLocationsForLocationIds(idList(request, "locations"));
    } else {
      return badRequest("Provide either tokens or locations");
    }
    return ok(collection ? proto::convert::toProto(*collection) : sourcetrail::SourceLocationCollectionResponse{});
  });

  // -- Tooltips --------------------------------------------------------------

  server.route("GET", "/api/v1/tooltip", [this](const http::Request& request) {
    sourcetrail::TooltipInfoResponse response;
    if(request.query.contains("tokens")) {
      const TooltipInfo info = mStorageAccess->getTooltipInfoForTokenIds(
          idList(request, "tokens"), static_cast<TooltipOrigin>(request.getUInt("origin")));
      *response.mutable_info() = proto::convert::toProto(info);
    } else if(request.query.contains("locations")) {
      const TooltipInfo info = mStorageAccess->getTooltipInfoForSourceLocationIdsAndLocalSymbolIds(
          idList(request, "locations"), idList(request, "locals"));
      *response.mutable_info() = proto::convert::toProto(info);
    } else {
      return badRequest("Provide either tokens or locations");
    }
    return ok(response);
  });

  // -- Errors ----------------------------------------------------------------

  server.route("GET", "/api/v1/errors", [this](const http::Request& request) {
    const ErrorFilter filter = errorFilterFrom(request);
    const std::string file = request.get("file");

    sourcetrail::ErrorInfosResponse response;
    const std::vector<ErrorInfo> errors = file.empty() ? mStorageAccess->getErrorsLimited(filter) :
                                                         mStorageAccess->getErrorsForFileLimited(filter, FilePath(fromUtf8(file)));
    for(const ErrorInfo& error : errors) {
      *response.add_errors() = proto::convert::toProto(error);
    }
    return ok(response);
  });

  // Carries only the four fields the location synthesis actually reads, not whole ErrorInfo rows:
  // the error messages used to cross the boundary just to be echoed back.
  server.route("POST", "/api/v1/errors/locations", [this](const http::Request& request) {
    sourcetrail::ErrorLocationsRequest body;
    if(!parseBody(request, body)) {
      return badRequest("Malformed error locations request");
    }

    std::vector<ErrorInfo> errors;
    errors.reserve(static_cast<size_t>(body.errors_size()));
    for(const auto& entry : body.errors()) {
      ErrorInfo error;
      error.id = entry.id();
      error.filePath = fromUtf8(entry.file_path());
      error.lineNumber = entry.line_number();
      error.columnNumber = entry.column_number();
      errors.push_back(error);
    }

    const auto collection = mStorageAccess->getErrorSourceLocations(errors);
    return ok(collection ? proto::convert::toProto(*collection) : sourcetrail::SourceLocationCollectionResponse{});
  });

  // -- Bookmarks -------------------------------------------------------------

  server.route("GET", "/api/v1/bookmarks", [this](const http::Request&) {
    sourcetrail::BookmarksResponse response;
    for(const NodeBookmark& bookmark : mStorageAccess->getAllNodeBookmarks()) {
      *response.add_node_bookmarks() = proto::convert::toProto(bookmark);
    }
    for(const EdgeBookmark& bookmark : mStorageAccess->getAllEdgeBookmarks()) {
      *response.add_edge_bookmarks() = proto::convert::toProto(bookmark);
    }
    for(const BookmarkCategory& category : mStorageAccess->getAllBookmarkCategories()) {
      *response.add_categories() = proto::convert::toProto(category);
    }
    return ok(response);
  });

  server.route("POST", "/api/v1/bookmarks", [this](const http::Request& request) {
    sourcetrail::AddBookmarkRequest body;
    if(!parseBody(request, body)) {
      return badRequest("Malformed bookmark");
    }

    // The same two defaults BookmarkController::createBookmark applies, which a client of this
    // route would otherwise have to know to send itself. Without the category, category_id
    // stays 0 and the row violates the bookmark table's foreign key, so the insert is rolled
    // back and the caller is handed an id for a bookmark that does not exist. Without the
    // timestamp, the row stores "not-a-date-time", which then throws on every later read --
    // poisoning the whole bookmark list for that project.
    sourcetrail::ProtoBookmark base = body.base();
    if(base.category().name().empty()) {
      base.mutable_category()->set_name(toUtf8(std::wstring{BookmarkDefaultCategoryName}));
    }
    if(base.timestamp().empty()) {
      base.set_timestamp(TimeStamp::now().toString());
    }

    sourcetrail::BookmarkIdResponse response;
    if(body.edge_ids_size() > 0) {
      sourcetrail::ProtoEdgeBookmark bookmark;
      *bookmark.mutable_base() = base;
      *bookmark.mutable_edge_ids() = body.edge_ids();
      bookmark.set_active_node_id(body.active_node_id());
      response.set_id(mStorageAccess->addEdgeBookmark(proto::convert::fromProto(bookmark)));
    } else {
      sourcetrail::ProtoNodeBookmark bookmark;
      *bookmark.mutable_base() = base;
      *bookmark.mutable_node_ids() = body.node_ids();
      response.set_id(mStorageAccess->addNodeBookmark(proto::convert::fromProto(bookmark)));
    }
    return ok(response);
  });

  server.route("PATCH", "/api/v1/bookmarks/", [this](const http::Request& request) {
    sourcetrail::UpdateBookmarkRequest body;
    if(!parseBody(request, body)) {
      return badRequest("Malformed bookmark update");
    }
    Id bookmarkId = static_cast<Id>(body.bookmark_id());
    if(bookmarkId == 0) {
      try {
        bookmarkId = static_cast<Id>(std::stoull(request.param));
      } catch(...) {
        return badRequest("Missing bookmark id");
      }
    }
    // Same substitution as the create route above: an empty category name would leave the row
    // pointing at no category at all.
    const std::wstring categoryName = body.category_name().empty() ? std::wstring{BookmarkDefaultCategoryName} :
                                                                     fromUtf8(body.category_name());
    mStorageAccess->updateBookmark(bookmarkId, fromUtf8(body.name()), fromUtf8(body.comment()), categoryName);
    return okEmpty();
  });

  // Registered before the plain bookmark delete; the longer prefix wins, so a category id is never
  // mistaken for a bookmark id.
  server.route("DELETE", "/api/v1/bookmarks/categories/", [this](const http::Request& request) {
    try {
      mStorageAccess->removeBookmarkCategory(static_cast<Id>(std::stoull(request.param)));
    } catch(...) {
      return badRequest("Missing bookmark category id");
    }
    return okEmpty();
  });

  server.route("DELETE", "/api/v1/bookmarks/", [this](const http::Request& request) {
    try {
      mStorageAccess->removeBookmark(static_cast<Id>(std::stoull(request.param)));
    } catch(...) {
      return badRequest("Missing bookmark id");
    }
    return okEmpty();
  });
}
