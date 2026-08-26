#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace http {

struct ClientResponse {
  static constexpr int StatusSuccessBegin = 200;
  static constexpr int StatusSuccessEnd = 300;

  int status{0};
  std::string body;

  [[nodiscard]] bool ok() const {
    return status >= StatusSuccessBegin && status < StatusSuccessEnd;
  }
};

/**
 * Synchronous HTTP/1.1 client for the engine boundary.
 *
 * Holds one keep-alive connection and re-dials it transparently, so the common case is a single
 * round trip on an already-open socket. Not thread-safe: callers that talk to the engine from
 * several threads give each thread its own Client.
 */
class Client {
public:
  Client(std::string host, uint16_t port, std::string authToken);
  ~Client();

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  Client(Client&&) noexcept;
  Client& operator=(Client&&) noexcept;

  /** Points this client at a different engine; drops the current connection. */
  void reconnect(std::string host, uint16_t port, std::string authToken);

  /** Returns nullopt when the engine is unreachable or the deadline passes. */
  std::optional<ClientResponse> request(const std::string& method,
                                        const std::string& target,
                                        const std::string& body,
                                        std::chrono::milliseconds timeout);

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

/**
 * Reader for one server-sent event stream.
 *
 * The engine pushes indexing progress, status and dialog questions this way. run() blocks for the
 * life of the connection, so callers give it a thread of its own; cancel() closes the socket from
 * another thread to break the blocking read.
 */
class EventSource {
public:
  using EventHandler = std::function<void(const std::string& name, const std::string& data)>;

  EventSource(std::string host, uint16_t port, std::string authToken);
  ~EventSource();

  EventSource(const EventSource&) = delete;
  EventSource& operator=(const EventSource&) = delete;
  EventSource(EventSource&&) = delete;
  EventSource& operator=(EventSource&&) = delete;

  void reconnect(std::string host, uint16_t port, std::string authToken);

  /**
   * Connects and dispatches every frame to `onEvent` until the stream ends or cancel() is called.
   * Returns false when the connection could not be established, which is the normal state while the
   * engine is restarting.
   */
  bool run(const std::string& target, const EventHandler& onEvent);

  /** Breaks a blocked run(); safe to call from another thread. */
  void cancel();

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

}    // namespace http
