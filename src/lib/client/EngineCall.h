#pragma once
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "EngineChannel.h"
#include "GlobalId.hpp"
#include "logging.h"
#include "ProtoJson.h"

namespace client {

/**
 * Serves a call without a socket, set when the engine is hosted in this same process.
 *
 * Returns the response body, or nullopt when the engine answered a non-2xx status. Installed by the
 * process that hosts the engine (the GUI does, by default); with none installed and no channel a
 * call simply finds no engine, which is read-only mode.
 */
using LocalDispatch = std::function<std::optional<std::string>(const std::string& method, const std::string& target,
                                                               const std::string& body)>;
void setLocalDispatch(LocalDispatch dispatch);
[[nodiscard]] const LocalDispatch& localDispatch();

/**
 * The one place an engine failure becomes an empty answer.
 *
 * Returns nullopt when the engine is unreachable, misses the deadline, answers a non-2xx status, or
 * sends a body that is not the expected message. The deadline matters as much as the error handling:
 * a UI thread blocked on a hung engine is indistinguishable from a frozen application.
 *
 * `timeout` overrides the channel default for the rare call that legitimately takes longer than a UI
 * is willing to wait -- loading a project opens the database and builds its caches.
 */
template <class Response>
std::optional<Response> call(EngineChannel* channel,
                             const char* what,
                             const std::string& method,
                             const std::string& target,
                             const std::string& body = {},
                             std::optional<std::chrono::milliseconds> timeout = std::nullopt) {
  if(channel == nullptr) {
    if(const LocalDispatch& local = localDispatch()) {
      const std::optional<std::string> responseBody = local(method, target, body);
      Response localMessage;
      if(!responseBody || !proto::json::fromJson(*responseBody, localMessage)) {
        LOG_WARNING(std::string("In-process engine call ") + what + " failed.");
        return std::nullopt;
      }
      return localMessage;
    }
    return std::nullopt;
  }

  const std::optional<http::ClientResponse> response = channel->send(
      method, target, body, timeout.value_or(channel->getCallTimeout()));

  if(!response) {
    LOG_WARNING(std::string("Engine call ") + what + " could not reach " + channel->getEndpoint());
    channel->markDegraded();
    return std::nullopt;
  }

  if(!response->ok()) {
    LOG_WARNING(std::string("Engine call ") + what + " failed with status " + std::to_string(response->status) + ": " +
                response->body);
    // A 4xx is the engine answering, so the connection is healthy even though this call was refused.
    // Only a transport failure means the engine is gone.
    channel->markConnected();
    return std::nullopt;
  }

  Response message;
  if(!proto::json::fromJson(response->body, message)) {
    LOG_WARNING(std::string("Engine call ") + what + " returned a body that could not be parsed.");
    channel->markConnected();
    return std::nullopt;
  }

  channel->markConnected();
  return message;
}

/** Fire-and-forget variant for endpoints whose response carries nothing. */
inline bool callVoid(EngineChannel* channel,
                     const char* what,
                     const std::string& method,
                     const std::string& target,
                     const std::string& body = {},
                     std::optional<std::chrono::milliseconds> timeout = std::nullopt) {
  if(channel == nullptr) {
    if(const LocalDispatch& local = localDispatch()) {
      return local(method, target, body).has_value();
    }
    return false;
  }

  const std::optional<http::ClientResponse> response = channel->send(
      method, target, body, timeout.value_or(channel->getCallTimeout()));

  if(!response) {
    LOG_WARNING(std::string("Engine call ") + what + " could not reach " + channel->getEndpoint());
    channel->markDegraded();
    return false;
  }

  channel->markConnected();
  if(!response->ok()) {
    LOG_WARNING(std::string("Engine call ") + what + " failed with status " + std::to_string(response->status));
    return false;
  }
  return true;
}

/** Percent-encodes everything that is not unreserved, so a path segment can carry a file path. */
std::string urlEncode(const std::string& text);

/** Renders ids as the comma-separated list the query parameters use. */
std::string joinIds(const std::vector<Id>& ids);

}    // namespace client
