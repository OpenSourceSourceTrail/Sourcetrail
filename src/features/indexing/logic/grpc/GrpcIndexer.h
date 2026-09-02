#pragma once
#include <string>

#include "GlobalId.hpp"

namespace grpc_indexer {
/**
 * Message size limit for the engine <-> worker boundary, applied on both ends.
 *
 * gRPC defaults the receive limit to 4 MB, and PushIntermediateStorage carries a whole
 * translation unit's IntermediateStorage in one unary message. A large TU therefore came back
 * RESOURCE_EXHAUSTED and its symbols were dropped, leaving that file missing from the index.
 * -1 is gRPC's "unlimited": the peer here is our own worker process on loopback, and the
 * indexing pipeline already trusts it with everything it parses.
 */
constexpr int UnlimitedMessageSize = -1;
}    // namespace grpc_indexer

class GrpcIndexer {
public:
  GrpcIndexer(std::string engineEndpoint, Id processId);

  void work();

private:
  std::string mEngineEndpoint;
  Id mProcessId;
};
