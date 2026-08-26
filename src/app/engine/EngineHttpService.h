#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>

#include <condition_variable>

#include "HttpServer.h"

namespace sourcetrail {
class DialogRequest;
class EngineEvent;
}    // namespace sourcetrail

class StorageAccess;

/**
 * The engine's side of the client boundary, served over HTTP + JSON.
 *
 * Replaces the former gRPC EngineServiceImpl. The handlers are the same storage calls; what changed
 * is that endpoints which the client always used together now answer in one response -- opening a
 * file was four round trips, the bookmark view was three plus five per active edge.
 */
class EngineHttpService {
public:
  explicit EngineHttpService(StorageAccess* storageAccess);
  ~EngineHttpService();

  EngineHttpService(const EngineHttpService&) = delete;
  EngineHttpService& operator=(const EngineHttpService&) = delete;
  EngineHttpService(EngineHttpService&&) = delete;
  EngineHttpService& operator=(EngineHttpService&&) = delete;

  /** Installs every route, plus the event stream, on `server`. */
  void registerRoutes(http::Server& server);

  void setShutdownHandler(std::function<void()> handler);

  /**
   * Pushes an event to every connected client.
   *
   * Kept to the signature the former gRPC service had, so EngineEventPublisher and EngineDialogView
   * -- which translate the engine's process-local bus and dialog calls into wire events -- did not
   * have to change shape.
   */
  void broadcastEvent(const sourcetrail::EngineEvent& event);

  /**
   * Asks the connected client a question and blocks until it answers.
   *
   * Indexing genuinely cannot continue without the answer (it decides whether the freshly built
   * database replaces the project's), which a one-way event stream cannot express. The question goes
   * out as a `dialog` event and the answer arrives as POST /api/v1/dialogs/{id}.
   *
   * Returns nullopt when no client answers before `timeout`, or when none is connected -- callers
   * then fall back to their headless default rather than hanging the engine forever.
   */
  std::optional<int> askDialog(const sourcetrail::DialogRequest& request, std::chrono::milliseconds timeout);

  /** Releases every thread blocked in askDialog; called when the engine is shutting down. */
  void abortDialogs();

private:
  struct PendingDialog {
    std::optional<int> answer;
  };

  http::Response handleDialogResponse(const http::Request& request);

  StorageAccess* mStorageAccess;
  http::Server* mServer{nullptr};
  std::function<void()> mShutdownHandler;

  std::mutex mDialogMutex;
  std::condition_variable mDialogSignal;
  std::map<uint64_t, PendingDialog> mPendingDialogs;
  uint64_t mNextDialogId{1};
  bool mDialogsAborted{false};
};
