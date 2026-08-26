#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "engine.pb.h"

class EngineChannel;

namespace http {
class EventSource;
}

namespace client {

/**
 * The client's half of the event stream: everything the engine reports while it works.
 *
 * Without it a client can only ask questions -- indexing progress, status lines and the "the index
 * is ready" signal all originate on the engine's message bus, which stops at the process boundary.
 * One thread holds a server-sent event stream open and re-dispatches each event onto the local bus,
 * so controllers and views carry on believing the work happens in-process.
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
  void handleFrame(const std::string& name, const std::string& data);

  EngineChannel* mChannel;
  std::atomic<bool> mStopping{true};
  std::thread mThread;

  // run() blocks inside the source; cancelling it is the only way to get the thread back.
  std::mutex mSourceMutex;
  std::shared_ptr<http::EventSource> mSource;
};

}    // namespace client
