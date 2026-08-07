#pragma once
#include <memory>

#include "engine.pb.h"

class SourceLocationCollection;
class SourceLocationFile;

namespace proto::convert {

/**
 * SourceLocationFile / SourceLocationCollection <-> proto.
 *
 * A SourceLocation is stored as a *pair* of entries (a start and an end that point at each other),
 * but the wire format carries one entry per logical location with both line/column pairs, matching
 * SourceLocationFile::addSourceLocation's signature. Serialization therefore walks start locations
 * only and reads the end from getOtherLocation().
 */
sourcetrail::ProtoSourceLocationFile toProto(const SourceLocationFile& file);
std::shared_ptr<SourceLocationFile> fromProto(const sourcetrail::ProtoSourceLocationFile& msg);

sourcetrail::SourceLocationCollectionResponse toProto(const SourceLocationCollection& collection);
std::shared_ptr<SourceLocationCollection> fromProto(const sourcetrail::SourceLocationCollectionResponse& msg);

}    // namespace proto::convert
