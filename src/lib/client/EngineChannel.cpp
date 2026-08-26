#include "EngineChannel.h"

#include <atomic>
#include <map>
#include <thread>
#include <utility>

#include "logging.h"

namespace {

/** Splits "host:port"; port 0 means the endpoint was unusable. */
std::pair<std::string, uint16_t> splitEndpoint(const std::string& endpoint) {
  const size_t colon = endpoint.rfind(':');
  if(colon == std::string::npos) {
    return {endpoint, 0};
  }
  try {
    return {endpoint.substr(0, colon), static_cast<uint16_t>(std::stoi(endpoint.substr(colon + 1)))};
  } catch(...) {
    return {endpoint.substr(0, colon), 0};
  }
}

struct CachedClient {
  uint64_t generation = 0;
  std::shared_ptr<http::Client> client;
};

/**
 * One keep-alive connection per calling thread, per channel.
 *
 * Per thread because http::Client is not thread-safe and a hover on a background thread must not
 * queue behind a graph query on the UI thread. Thread-local rather than a map inside the channel so
 * that a thread's connection is closed when that thread exits -- indexing answers its dialog on a
 * short-lived thread, which would otherwise leave an open socket behind for the life of the process.
 */
thread_local std::map<uint64_t, CachedClient> tClients;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

std::atomic<uint64_t> sNextInstanceId{1};

}    // namespace

EngineChannel::EngineChannel(std::string endpoint, std::string authToken)
    : mEndpoint(std::move(endpoint)), mAuthToken(std::move(authToken)), mInstanceId(sNextInstanceId++) {
  mPort = splitEndpoint(mEndpoint).second;
}

EngineChannel::~EngineChannel() = default;

const std::string& EngineChannel::getEndpoint() const {
  return mEndpoint;
}

std::string EngineChannel::getAuthToken() const {
  const std::lock_guard<std::mutex> lock(mMutex);
  return mAuthToken;
}

uint16_t EngineChannel::getPort() const {
  const std::lock_guard<std::mutex> lock(mMutex);
  return mPort;
}

void EngineChannel::reconnect(std::string endpoint, std::string authToken) {
  {
    const std::lock_guard<std::mutex> lock(mMutex);
    mEndpoint = std::move(endpoint);
    mAuthToken = std::move(authToken);
    mPort = splitEndpoint(mEndpoint).second;
    // Bumping the generation is what retires the pooled connections: a request already running on
    // another thread keeps its own shared_ptr and finishes against the old engine rather than
    // crashing, and every later request rebuilds against the new one.
    ++mGeneration;
  }
  setConnected(false);
}

std::shared_ptr<http::Client> EngineChannel::clientForThisThread() {
  std::string host;
  uint16_t port = 0;
  std::string token;
  uint64_t generation = 0;
  {
    const std::lock_guard<std::mutex> lock(mMutex);
    if(mPort == 0) {
      return nullptr;
    }
    host = splitEndpoint(mEndpoint).first;
    port = mPort;
    token = mAuthToken;
    generation = mGeneration;
  }

  CachedClient& cached = tClients[mInstanceId];
  if(cached.client && cached.generation == generation) {
    return cached.client;
  }

  cached.client = std::make_shared<http::Client>(std::move(host), port, std::move(token));
  cached.generation = generation;
  return cached.client;
}

std::optional<http::ClientResponse> EngineChannel::send(const std::string& method,
                                                        const std::string& target,
                                                        const std::string& body,
                                                        std::chrono::milliseconds timeout) {
  const std::shared_ptr<http::Client> client = clientForThisThread();
  if(!client) {
    return std::nullopt;
  }
  return client->request(method, target, body, timeout);
}

bool EngineChannel::waitUntilReady(std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;

  // There is no connection state to observe as there was with a gRPC channel, so readiness is
  // whether the engine answers its cheapest endpoint.
  while(std::chrono::steady_clock::now() < deadline) {
    if(const auto response = send("GET", "/api/v1/capabilities", {}, getCallTimeout()); response && response->ok()) {
      setConnected(true);
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  setConnected(false);
  return false;
}

bool EngineChannel::isConnected() const {
  const std::lock_guard<std::mutex> lock(mMutex);
  return mConnected;
}

void EngineChannel::markConnected() {
  setConnected(true);
}

void EngineChannel::markDegraded() {
  setConnected(false);
}

void EngineChannel::setConnectionStateHandler(std::function<void(bool)> handler) {
  const std::lock_guard<std::mutex> lock(mMutex);
  mConnectionStateHandler = std::move(handler);
}

std::chrono::milliseconds EngineChannel::getCallTimeout() const {
  const std::lock_guard<std::mutex> lock(mMutex);
  return mCallTimeout;
}

void EngineChannel::setCallTimeout(std::chrono::milliseconds timeout) {
  const std::lock_guard<std::mutex> lock(mMutex);
  mCallTimeout = timeout;
}

void EngineChannel::setConnected(bool connected) {
  std::function<void(bool)> handler;
  std::string endpoint;
  {
    const std::lock_guard<std::mutex> lock(mMutex);
    if(mConnected == connected) {
      return;
    }
    mConnected = connected;
    handler = mConnectionStateHandler;
    endpoint = mEndpoint;
  }

  LOG_INFO(connected ? "Engine connection established: " + endpoint : "Engine connection lost: " + endpoint);

  // Invoked outside the lock: the handler is the supervisor, which may respawn the engine and call
  // straight back into reconnect().
  if(handler) {
    handler(connected);
  }
}
