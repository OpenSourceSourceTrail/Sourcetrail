#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "HttpClient.h"

/**
 * The client's half of the connection to sourcetrail_engine: holds the endpoint and bearer token,
 * and tracks whether the engine is currently answering.
 *
 * A client must keep working while the engine is down -- restarting it is the supervisor's job, not
 * the caller's. Every request therefore goes through here, which reports failures via markDegraded()
 * so one observer (the supervisor) can react while every caller stays oblivious.
 */
class EngineChannel final {
public:
  static constexpr std::chrono::milliseconds DefaultCallTimeout{10000};

  /** `endpoint` is "host:port"; `authToken` is the value the engine printed on its handshake line. */
  EngineChannel(std::string endpoint, std::string authToken);
  ~EngineChannel();

  EngineChannel(const EngineChannel&) = delete;
  EngineChannel& operator=(const EngineChannel&) = delete;
  EngineChannel(EngineChannel&&) = delete;
  EngineChannel& operator=(EngineChannel&&) = delete;

  /** Endpoint this channel talks to, e.g. "127.0.0.1:41234". */
  [[nodiscard]] const std::string& getEndpoint() const;
  [[nodiscard]] std::string getAuthToken() const;
  [[nodiscard]] uint16_t getPort() const;

  /**
   * Points the channel at a new endpoint and drops every pooled connection. Used after the
   * supervisor restarts the engine, which comes back up on a fresh port with a fresh token.
   */
  void reconnect(std::string endpoint, std::string authToken);

  /**
   * Polls the engine until it answers, at most for `timeout`. Returns false on timeout, which is a
   * normal outcome while the engine is still starting -- not an error.
   */
  bool waitUntilReady(std::chrono::milliseconds timeout);

  /**
   * Performs one request. Returns nullopt when the engine is unreachable or misses the deadline.
   *
   * Safe to call from several threads at once: each thread gets its own keep-alive connection,
   * because a hover on a background thread must not queue behind a graph query on the UI thread.
   */
  std::optional<http::ClientResponse> send(const std::string& method,
                                           const std::string& target,
                                           const std::string& body,
                                           std::chrono::milliseconds timeout);

  /** False once a request has failed, until the next successful one. */
  [[nodiscard]] bool isConnected() const;

  void markConnected();
  void markDegraded();

  /** Invoked on every isConnected() transition, with the new value. */
  void setConnectionStateHandler(std::function<void(bool)> handler);

  /** Deadline applied to every request, so a hung engine cannot freeze a UI thread. */
  [[nodiscard]] std::chrono::milliseconds getCallTimeout() const;
  void setCallTimeout(std::chrono::milliseconds timeout);

private:
  void setConnected(bool connected);

  /** The calling thread's client, rebuilt whenever reconnect() has bumped the generation. */
  std::shared_ptr<http::Client> clientForThisThread();

  mutable std::mutex mMutex;
  std::string mEndpoint;
  std::string mAuthToken;
  uint16_t mPort{0};

  // Bumped by reconnect(); a thread whose cached client predates it rebuilds against the new engine.
  uint64_t mGeneration{0};
  // Identifies this channel in the thread-local client cache. Not the address: a destroyed channel's
  // address can be reused, which would hand a new channel the old one's connections.
  uint64_t mInstanceId;

  bool mConnected = false;
  std::function<void(bool)> mConnectionStateHandler;

  std::chrono::milliseconds mCallTimeout = DefaultCallTimeout;
};
