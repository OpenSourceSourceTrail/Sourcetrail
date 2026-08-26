#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <string_view>

namespace http {

struct Request {
  std::string method;
  std::string path;
  /** Decoded query parameters. */
  std::map<std::string, std::string> query;
  /** Path remainder captured by a prefix route (one registered with a trailing '/'). */
  std::string param;
  std::string body;

  /** Query value, or `fallback` when absent. */
  [[nodiscard]] std::string get(const std::string& key, const std::string& fallback = {}) const;
  [[nodiscard]] uint64_t getUInt(const std::string& key, uint64_t fallback = 0) const;
  [[nodiscard]] bool getBool(const std::string& key, bool fallback = false) const;
  /** Comma-separated query value split into its parts; empty when absent. */
  [[nodiscard]] std::vector<std::string> getList(const std::string& key) const;
};

struct Response {
  static constexpr int StatusOk = 200;

  int status{StatusOk};
  std::string body;
  std::string contentType{"application/json"};

  static Response json(std::string body);
  static Response error(int status, std::string_view message);
};

using Handler = std::function<Response(const Request&)>;

/**
 * Loopback HTTP/1.1 server for the client <-> engine boundary.
 *
 * One thread accepts, one thread serves each connection. That is the whole concurrency model: this
 * listens on 127.0.0.1 for a single GUI plus the occasional browser, so a connection count that
 * would justify async I/O cannot occur.
 * ponytail: thread per connection; move to an io_context pool if the client count ever grows.
 *
 * Every request must carry `Authorization: Bearer <token>`, and any request naming an `Origin` that
 * is not allow-listed is rejected -- a loopback HTTP port is reachable from any page in the user's
 * browser, which the gRPC port it replaces was not.
 */
class Server {
public:
  explicit Server(std::string authToken);
  ~Server();

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;
  Server(Server&&) noexcept;
  Server& operator=(Server&&) noexcept;

  /**
   * Registers a handler. A `path` ending in '/' matches every target beneath it and hands the
   * remainder to the handler as `Request::param`; any other path matches exactly. Exact matches win.
   */
  void route(std::string method, std::string path, Handler handler);

  /** Serves a server-sent event stream at `path`, fed by broadcast(). */
  void eventStream(std::string path);

  /** Queues an event on every open stream. Never blocks on a slow reader. */
  void broadcast(std::string_view name, std::string_view data);

  /** Allows browser requests from `origin`. Without this only non-browser clients are accepted. */
  void allowOrigin(std::string origin);

  /** The bearer token every request must present. */
  [[nodiscard]] const std::string& authToken() const;

  /** Binds and starts serving; returns the bound port (pass 0 to let the OS choose). 0 on failure. */
  uint16_t start(uint16_t port);

  void stop();

private:
  struct Impl;
  // Shared rather than unique: each connection is served on a detached thread that outlives the
  // call which spawned it, so those threads keep the state alive instead of borrowing it.
  std::shared_ptr<Impl> mImpl;
};

}    // namespace http
