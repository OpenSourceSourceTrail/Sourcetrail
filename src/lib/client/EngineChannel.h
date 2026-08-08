#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "engine.grpc.pb.h"

/**
 * The client's half of the connection to sourcetrail_engine: owns the gRPC channel and stub, and
 * tracks whether the engine is currently answering.
 *
 * A client must keep working while the engine is down -- restarting it is the supervisor's job, not
 * the caller's. Every RPC therefore goes through GrpcStorageAccess, which reports failures back here
 * via markDegraded() so one observer (the supervisor) can react while every caller stays oblivious.
 */
class EngineChannel final {
public:
  static constexpr std::chrono::milliseconds DefaultCallTimeout{10000};

  explicit EngineChannel(std::string endpoint);

  /** Endpoint this channel was built for, e.g. "127.0.0.1:41234". */
  [[nodiscard]] const std::string& getEndpoint() const;

  /**
   * Points the channel at a new endpoint and drops the old stub. Used after the supervisor restarts
   * the engine, which comes back up on a fresh port.
   */
  void reconnect(std::string endpoint);

  /**
   * Blocks until the channel reaches READY, at most for `timeout`. Returns false on timeout, which
   * is a normal outcome while the engine is still starting -- not an error.
   */
  bool waitUntilReady(std::chrono::milliseconds timeout);

  /** Null only before the first connect attempt; never dereference without checking. */
  [[nodiscard]] sourcetrail::EngineService::Stub* getStub() const;

  /** False once an RPC has failed, until the next successful one. */
  [[nodiscard]] bool isConnected() const;

  void markConnected();
  void markDegraded();

  /** Invoked on every isConnected() transition, with the new value. */
  void setConnectionStateHandler(std::function<void(bool)> handler);

  /** Deadline applied to every unary RPC, so a hung engine cannot freeze a UI thread. */
  [[nodiscard]] std::chrono::milliseconds getCallTimeout() const;
  void setCallTimeout(std::chrono::milliseconds timeout);

private:
  void setConnected(bool connected);

  std::string mEndpoint;
  std::shared_ptr<grpc::Channel> mChannel;
  std::unique_ptr<sourcetrail::EngineService::Stub> mStub;

  bool mConnected = false;
  std::function<void(bool)> mConnectionStateHandler;

  std::chrono::milliseconds mCallTimeout = DefaultCallTimeout;
};
