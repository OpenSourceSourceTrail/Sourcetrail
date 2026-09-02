#include "ConvertQuery.h"

#include "bookmark/domain/BookmarkCategory.h"
#include "bookmark/domain/EdgeBookmark.h"
#include "bookmark/domain/NodeBookmark.h"
#include "ConvertLocations.h"
#include "data/name/NameHierarchy.h"
#include "data/NodeType.h"
#include "data/NodeTypeSet.h"
#include "data/storage/StorageStats.h"
#include "error/domain/ErrorFilter.h"
#include "error/domain/ErrorInfo.h"
#include "FileInfo.h"
#include "search/domain/SearchMatch.h"
#include "tooltip/domain/TooltipInfo.h"
#include "utilityString.h"

namespace {

sourcetrail::ProtoBookmark bookmarkBaseToProto(const Bookmark& bookmark) {
  sourcetrail::ProtoBookmark msg;
  msg.set_id(bookmark.getId());
  msg.set_name(utility::encodeToUtf8(bookmark.getName()));
  msg.set_comment(utility::encodeToUtf8(bookmark.getComment()));
  msg.set_timestamp(bookmark.getTimeStamp().toString());
  *msg.mutable_category() = proto::convert::toProto(bookmark.getCategory());
  return msg;
}

}    // namespace

namespace proto::convert {

// ---- NodeTypeSet <-> NodeKindMask ------------------------------------------

// NodeKindMask is `typedef int`, so the bit twiddling is done on an unsigned copy.
NodeKindMask toNodeKindMask(const NodeTypeSet& types) {
  unsigned int mask = 0;
  for(const NodeType& type : types.getNodeTypes()) {
    mask |= static_cast<unsigned int>(nodeKindToInt(type.getKind()));
  }
  return static_cast<NodeKindMask>(mask);
}

NodeTypeSet nodeTypeSetFromMask(NodeKindMask mask) {
  const auto bits = static_cast<unsigned int>(mask);
  NodeTypeSet types = NodeTypeSet::none();
  for(unsigned int bit = 1; bit <= static_cast<unsigned int>(NODE_MAX_VALUE); bit *= 2) {
    if((bits & bit) != 0) {
      types.add(NodeTypeSet(NodeType(intToNodeKind(static_cast<int>(bit)))));
    }
  }
  return types;
}

// ---- SearchMatch -----------------------------------------------------------

sourcetrail::ProtoSearchMatch toProto(const SearchMatch& match) {
  sourcetrail::ProtoSearchMatch msg;
  msg.set_name(utility::encodeToUtf8(match.name));
  msg.set_text(utility::encodeToUtf8(match.text));
  msg.set_subtext(utility::encodeToUtf8(match.subtext));
  for(const Id tokenId : match.tokenIds) {
    msg.add_token_ids(tokenId);
  }
  for(const NameHierarchy& tokenName : match.tokenNames) {
    msg.add_token_names_serialized(utility::encodeToUtf8(NameHierarchy::serialize(tokenName)));
  }
  msg.set_type_name(utility::encodeToUtf8(match.typeName));
  msg.set_node_kind(nodeKindToInt(match.nodeType.getKind()));
  msg.set_search_type(static_cast<int32_t>(match.searchType));
  for(const size_t index : match.indices) {
    msg.add_indices(index);
  }
  msg.set_score(match.score);
  msg.set_has_children(match.hasChildren);
  // Carried for readability of the wire format only: SearchMatch::getCommandType() derives it from
  // `name`, so there is no setter to restore it into and fromProto ignores it.
  msg.set_command_type(static_cast<int32_t>(match.getCommandType()));
  return msg;
}

SearchMatch fromProto(const sourcetrail::ProtoSearchMatch& msg) {
  SearchMatch match;
  match.name = utility::decodeFromUtf8(msg.name());
  match.text = utility::decodeFromUtf8(msg.text());
  match.subtext = utility::decodeFromUtf8(msg.subtext());
  match.tokenIds.assign(msg.token_ids().begin(), msg.token_ids().end());
  for(const auto& serialized : msg.token_names_serialized()) {
    match.tokenNames.push_back(NameHierarchy::deserialize(utility::decodeFromUtf8(serialized)));
  }
  match.typeName = utility::decodeFromUtf8(msg.type_name());
  match.nodeType = NodeType(intToNodeKind(msg.node_kind()));
  match.searchType = static_cast<SearchMatch::SearchType>(msg.search_type());
  match.indices.assign(msg.indices().begin(), msg.indices().end());
  match.score = msg.score();
  match.hasChildren = msg.has_children();
  return match;
}

// ---- Errors ----------------------------------------------------------------

sourcetrail::ProtoErrorInfo toProto(const ErrorInfo& error) {
  sourcetrail::ProtoErrorInfo msg;
  msg.set_id(error.id);
  msg.set_message(utility::encodeToUtf8(error.message));
  msg.set_file_path(utility::encodeToUtf8(error.filePath));
  msg.set_line_number(error.lineNumber);
  msg.set_column_number(error.columnNumber);
  msg.set_translation_unit(utility::encodeToUtf8(error.translationUnit));
  msg.set_fatal(error.fatal);
  msg.set_indexed(error.indexed);
  return msg;
}

ErrorInfo fromProto(const sourcetrail::ProtoErrorInfo& msg) {
  return {msg.id(),
          utility::decodeFromUtf8(msg.message()),
          utility::decodeFromUtf8(msg.file_path()),
          msg.line_number(),
          msg.column_number(),
          utility::decodeFromUtf8(msg.translation_unit()),
          msg.fatal(),
          msg.indexed()};
}

sourcetrail::ProtoErrorFilter toProto(const ErrorFilter& filter) {
  sourcetrail::ProtoErrorFilter msg;
  msg.set_error(filter.error);
  msg.set_fatal(filter.fatal);
  msg.set_unindexed_error(filter.unindexedError);
  msg.set_unindexed_fatal(filter.unindexedFatal);
  msg.set_limit(filter.limit);
  return msg;
}

ErrorFilter fromProto(const sourcetrail::ProtoErrorFilter& msg) {
  ErrorFilter filter;
  filter.error = msg.error();
  filter.fatal = msg.fatal();
  filter.unindexedError = msg.unindexed_error();
  filter.unindexedFatal = msg.unindexed_fatal();
  filter.limit = msg.limit();
  return filter;
}

// ---- Files and stats -------------------------------------------------------

sourcetrail::ProtoFileInfo toProto(const FileInfo& info) {
  sourcetrail::ProtoFileInfo msg;
  msg.set_file_path(utility::encodeToUtf8(info.path.wstr()));
  msg.set_last_write_time(info.lastWriteTime.toString());
  return msg;
}

FileInfo fromProto(const sourcetrail::ProtoFileInfo& msg) {
  return {FilePath(utility::decodeFromUtf8(msg.file_path())), TimeStamp(msg.last_write_time())};
}

sourcetrail::ProtoStorageStats toProto(const StorageStats& stats) {
  sourcetrail::ProtoStorageStats msg;
  msg.set_node_count(stats.nodeCount);
  msg.set_edge_count(stats.edgeCount);
  msg.set_file_count(stats.fileCount);
  msg.set_completed_file_count(stats.completedFileCount);
  msg.set_file_loc_count(stats.fileLOCCount);
  msg.set_timestamp(stats.timestamp.toString());
  return msg;
}

StorageStats fromProto(const sourcetrail::ProtoStorageStats& msg) {
  StorageStats stats;
  stats.nodeCount = msg.node_count();
  stats.edgeCount = msg.edge_count();
  stats.fileCount = msg.file_count();
  stats.completedFileCount = msg.completed_file_count();
  stats.fileLOCCount = msg.file_loc_count();
  stats.timestamp = TimeStamp(msg.timestamp());
  return stats;
}

// ---- Bookmarks -------------------------------------------------------------

sourcetrail::ProtoBookmarkCategory toProto(const BookmarkCategory& category) {
  sourcetrail::ProtoBookmarkCategory msg;
  msg.set_id(category.getId());
  msg.set_name(utility::encodeToUtf8(category.getName()));
  return msg;
}

BookmarkCategory fromProto(const sourcetrail::ProtoBookmarkCategory& msg) {
  return {msg.id(), utility::decodeFromUtf8(msg.name())};
}

sourcetrail::ProtoNodeBookmark toProto(const NodeBookmark& bookmark) {
  sourcetrail::ProtoNodeBookmark msg;
  *msg.mutable_base() = bookmarkBaseToProto(bookmark);
  for(const Id nodeId : bookmark.getNodeIds()) {
    msg.add_node_ids(nodeId);
  }
  return msg;
}

NodeBookmark fromProto(const sourcetrail::ProtoNodeBookmark& msg) {
  const auto& base = msg.base();
  NodeBookmark bookmark(base.id(),
                        utility::decodeFromUtf8(base.name()),
                        utility::decodeFromUtf8(base.comment()),
                        TimeStamp(base.timestamp()),
                        fromProto(base.category()));
  bookmark.setNodeIds(std::vector<Id>(msg.node_ids().begin(), msg.node_ids().end()));
  return bookmark;
}

sourcetrail::ProtoEdgeBookmark toProto(const EdgeBookmark& bookmark) {
  sourcetrail::ProtoEdgeBookmark msg;
  *msg.mutable_base() = bookmarkBaseToProto(bookmark);
  for(const Id edgeId : bookmark.getEdgeIds()) {
    msg.add_edge_ids(edgeId);
  }
  msg.set_active_node_id(bookmark.getActiveNodeId());
  return msg;
}

EdgeBookmark fromProto(const sourcetrail::ProtoEdgeBookmark& msg) {
  const auto& base = msg.base();
  EdgeBookmark bookmark(base.id(),
                        utility::decodeFromUtf8(base.name()),
                        utility::decodeFromUtf8(base.comment()),
                        TimeStamp(base.timestamp()),
                        fromProto(base.category()));
  bookmark.setEdgeIds(std::vector<Id>(msg.edge_ids().begin(), msg.edge_ids().end()));
  bookmark.setActiveNodeId(msg.active_node_id());
  return bookmark;
}

// ---- Tooltips --------------------------------------------------------------

sourcetrail::ProtoTooltipInfo toProto(const TooltipInfo& info) {
  sourcetrail::ProtoTooltipInfo msg;
  msg.set_title(utility::encodeToUtf8(info.title));
  msg.set_count(info.count);
  msg.set_count_text(info.countText);
  for(const TooltipSnippet& snippet : info.snippets) {
    auto* snippetMsg = msg.add_snippets();
    snippetMsg->set_code(utility::encodeToUtf8(snippet.code));
    if(snippet.locationFile) {
      *snippetMsg->mutable_location_file() = toProto(*snippet.locationFile);
    }
  }
  msg.set_offset_x(info.offset.x);
  msg.set_offset_y(info.offset.y);
  return msg;
}

TooltipInfo fromProto(const sourcetrail::ProtoTooltipInfo& msg) {
  TooltipInfo info;
  info.title = utility::decodeFromUtf8(msg.title());
  info.count = msg.count();
  info.countText = msg.count_text();
  for(const auto& snippetMsg : msg.snippets()) {
    TooltipSnippet snippet;
    snippet.code = utility::decodeFromUtf8(snippetMsg.code());
    if(snippetMsg.has_location_file()) {
      snippet.locationFile = fromProto(snippetMsg.location_file());
    }
    info.snippets.push_back(std::move(snippet));
  }
  info.offset = Vec2f(msg.offset_x(), msg.offset_y());
  return info;
}

}    // namespace proto::convert
