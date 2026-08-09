#pragma once
#include <atomic>
#include <mutex>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "engine.pb.h"

class EngineChannel;

namespace client {

/**
 * The client's half of the event stream: everything the engine reports while it works.
 *
 * Without it a client can only ask questions -- indexing progress, status lines and the "the index
 * is ready" signal all originate on the engine's message bus, which stops at the process boundary.
 * One thread holds a WatchEvents stream open and re-dispatches each event onto the local bus, so
 * controllers and views carry on believing the work happens in-process.
 *
 * The stream dies whenever the engine does; the loop simply reopens it, which is also how it picks
 * up the new port after QtEngineSupervisor respawns the engine.
 */
class EngineEventClient final {
public:
  explicit EngineEventClient(EngineChannel* channel);
  ~EngineEventClient();

  EngineEventClient(const EngineEventClient&) = delete;
  EngineEventClient& operator=(const EngineEventClient&) = delete;
  EngineEventClient(EngineEventClient&&) = delete;
  EngineEventClient& operator=(EngineEventClient&&) = delete;

  void start();
  void stop();

  /** Translates one event into local messages and dialog calls. Public for testing. */
  static void apply(const sourcetrail::EngineEvent& event);

private:
  void run();

  EngineChannel* mChannel;
  std::atomic<bool> mStopping{true};
  std::thread mThread;

  // Read() blocks; cancelling the context is the only way to get the thread back.
  std::mutex mContextMutex;
  grpc::ClientContext* mContext = nullptr;
};

}    // namespace client
