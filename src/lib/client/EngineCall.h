#pragma once
#include <optional>
#include <string>

#include <grpcpp/grpcpp.h>

#include "EngineChannel.h"
#include "engine.grpc.pb.h"
#include "logging.h"

namespace client {

using Stub = sourcetrail::EngineService::Stub;

template <class Request, class Response>
using UnaryRpc = grpc::Status (Stub::*)(grpc::ClientContext*, const Request&, Response*);

/**
 * The one place an engine failure becomes an empty answer.
 *
 * Returns nullopt when the engine is unreachable, misses the deadline, or answers non-OK. The
 * deadline matters as much as the error handling: a UI thread blocked on a hung engine is
 * indistinguishable from a frozen application.
 */
template <class Request, class Response>
std::optional<Response> call(EngineChannel* channel,
                             const char* rpcName,
                             UnaryRpc<Request, Response> method,
                             const Request& request) {
  if(channel == nullptr || channel->getStub() == nullptr) {
    return std::nullopt;
  }

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + channel->getCallTimeout());

  Response response;
  const grpc::Status status = (channel->getStub()->*method)(&context, request, &response);
  if(!status.ok()) {
    LOG_WARNING(std::string("Engine call ") + rpcName + " failed: " + status.error_message());
    channel->markDegraded();
    return std::nullopt;
  }

  channel->markConnected();
  return response;
}

}    // namespace client
