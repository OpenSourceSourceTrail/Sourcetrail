#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

class StorageAccess;
class EngineHttpService;
class EngineEventPublisher;

namespace http {
class Server;
}

/**
 * The engine, hosted inside whichever process wants it.
 *
 * `sourcetrail_engine` runs it as its whole reason to exist; the GUI links the same engine and runs
 * it in-process, serving HTTP only when asked to (--http-port), so a client such as the MCP server
 * can still reach the index. Both need the same source-group registration and the same
 * server-plus-publisher bring-up, which is why it lives here instead of in two mains.
 */
namespace engine_host {

/**
 * Registers the source-group modules and discovers the indexer plugins.
 *
 * Parsing a compilation database and compiling a precompiled header happen in the C/C++ indexer, so
 * the toolchain is the remote one and the hosting process links no language package. With no
 * indexer installed the toolchain answers nothing and the project stays browsable.
 *
 * Call after Application::createInstance.
 */
void registerSourceGroupModules();

/** A running HTTP endpoint in front of an in-process engine. */
class HttpEndpoint final {
public:
  /**
   * `broadcastOnly` keeps the publisher from installing its DialogView factory: in the GUI the real
   * Qt dialogs must stay in place, and only the event broadcast is wanted.
   */
  HttpEndpoint(StorageAccess* storageAccess, bool broadcastOnly);
  ~HttpEndpoint();

  HttpEndpoint(const HttpEndpoint&) = delete;
  HttpEndpoint& operator=(const HttpEndpoint&) = delete;
  HttpEndpoint(HttpEndpoint&&) = delete;
  HttpEndpoint& operator=(HttpEndpoint&&) = delete;

  /**
   * Serves one call from a client inside this process, bypassing the socket entirely. Returns the
   * response body, or nullopt when the engine refused the call.
   *
   * Works whether or not start() was ever called: hosting the engine in-process is the GUI default,
   * and it opens a port only when asked to.
   */
  [[nodiscard]] std::optional<std::string> callLocal(const std::string& method,
                                                     const std::string& target,
                                                     const std::string& body) const;

  /**
   * Starts listening and prints the `ENGINE_PORT <port> <token>` handshake line on stdout, flushed,
   * so a parent process that asked for port 0 can learn both. Returns the assigned port, or 0 when
   * the listener could not be started.
   */
  uint16_t start(uint16_t port);

  /** Releases anything blocked waiting for a dialog answer, then stops the listener. */
  void stop();

  [[nodiscard]] EngineHttpService& service() const;

private:
  std::unique_ptr<EngineHttpService> mService;
  std::unique_ptr<http::Server> mServer;
  std::unique_ptr<EngineEventPublisher> mPublisher;
};

}    // namespace engine_host
