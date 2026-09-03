#pragma once
#include "engine.pb.h"
#include "graph/domain/NodeKind.h"

class BookmarkCategory;
class NodeTypeSet;
class EdgeBookmark;
class NodeBookmark;
struct ErrorFilter;
struct ErrorInfo;
struct FileInfo;
struct SearchMatch;
struct StorageStats;
struct TooltipInfo;

namespace proto::convert {

/**
 * The plain-value query types of the client<->engine boundary: SearchMatch, ErrorInfo/ErrorFilter,
 * FileInfo, StorageStats, the bookmark trio and TooltipInfo.
 *
 * Graph and SourceLocation live in ConvertGraph.h / ConvertLocations.h; the storage POD types of the
 * indexer boundary live in Convert.h. As there, both directions are kept together so the server's
 * toProto and the client's fromProto cannot drift apart.
 */

/**
 * NodeTypeSet travels the wire as a NodeKind bitmask: NodeTypeSet's own mask constructor is private,
 * so neither side can rebuild one from a raw mask without this.
 */
NodeKindMask toNodeKindMask(const NodeTypeSet& types);
NodeTypeSet nodeTypeSetFromMask(NodeKindMask mask);

sourcetrail::ProtoSearchMatch toProto(const SearchMatch& match);
SearchMatch fromProto(const sourcetrail::ProtoSearchMatch& msg);

sourcetrail::ProtoErrorInfo toProto(const ErrorInfo& error);
ErrorInfo fromProto(const sourcetrail::ProtoErrorInfo& msg);

sourcetrail::ProtoErrorFilter toProto(const ErrorFilter& filter);
ErrorFilter fromProto(const sourcetrail::ProtoErrorFilter& msg);

sourcetrail::ProtoFileInfo toProto(const FileInfo& info);
FileInfo fromProto(const sourcetrail::ProtoFileInfo& msg);

sourcetrail::ProtoStorageStats toProto(const StorageStats& stats);
StorageStats fromProto(const sourcetrail::ProtoStorageStats& msg);

sourcetrail::ProtoBookmarkCategory toProto(const BookmarkCategory& category);
BookmarkCategory fromProto(const sourcetrail::ProtoBookmarkCategory& msg);

sourcetrail::ProtoNodeBookmark toProto(const NodeBookmark& bookmark);
NodeBookmark fromProto(const sourcetrail::ProtoNodeBookmark& msg);

sourcetrail::ProtoEdgeBookmark toProto(const EdgeBookmark& bookmark);
EdgeBookmark fromProto(const sourcetrail::ProtoEdgeBookmark& msg);

/**
 * TooltipInfo carries SourceLocationFiles inside its snippets, so this delegates to
 * ConvertLocations for those.
 */
sourcetrail::ProtoTooltipInfo toProto(const TooltipInfo& info);
TooltipInfo fromProto(const sourcetrail::ProtoTooltipInfo& msg);

}    // namespace proto::convert
