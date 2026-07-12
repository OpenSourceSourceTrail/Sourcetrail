#pragma once
#include <memory>

#include "sourcetrail_common.pb.h"

class IndexerCommand;
class IntermediateStorage;
struct StorageNode;
struct StorageEdge;
struct StorageFile;
struct StorageSymbol;
struct StorageLocalSymbol;
struct StorageSourceLocation;
struct StorageOccurrence;
struct StorageComponentAccess;
struct StorageError;

namespace proto::convert {

// Storage POD types
sourcetrail::StorageNode toProto(const StorageNode& node);
StorageNode fromProto(const sourcetrail::StorageNode& msg);

sourcetrail::StorageEdge toProto(const StorageEdge& edge);
StorageEdge fromProto(const sourcetrail::StorageEdge& msg);

sourcetrail::StorageFile toProto(const StorageFile& file);
StorageFile fromProto(const sourcetrail::StorageFile& msg);

sourcetrail::StorageSymbol toProto(const StorageSymbol& sym);
StorageSymbol fromProto(const sourcetrail::StorageSymbol& msg);

sourcetrail::StorageLocalSymbol toProto(const StorageLocalSymbol& sym);
StorageLocalSymbol fromProto(const sourcetrail::StorageLocalSymbol& msg);

sourcetrail::StorageSourceLocation toProto(const StorageSourceLocation& loc);
StorageSourceLocation fromProto(const sourcetrail::StorageSourceLocation& msg);

sourcetrail::StorageOccurrence toProto(const StorageOccurrence& occ);
StorageOccurrence fromProto(const sourcetrail::StorageOccurrence& msg);

sourcetrail::StorageComponentAccess toProto(const StorageComponentAccess& ca);
StorageComponentAccess fromProto(const sourcetrail::StorageComponentAccess& msg);

sourcetrail::StorageError toProto(const StorageError& err);
StorageError fromProto(const sourcetrail::StorageError& msg);

// IndexerCommand (polymorphic via dynamic_cast)
sourcetrail::IndexerCommand toProto(const IndexerCommand* cmd);
std::shared_ptr<IndexerCommand> fromProto(const sourcetrail::IndexerCommand& msg);

// IntermediateStorage bulk conversion
sourcetrail::IntermediateStorage toProto(const IntermediateStorage& storage);
std::shared_ptr<IntermediateStorage> fromProto(const sourcetrail::IntermediateStorage& msg);

}    // namespace proto::convert
