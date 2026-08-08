#include "EngineChannel.h"

#include <utility>

#include "logging.h"

namespace {

std::shared_ptr<grpc::Channel> createChannel(const std::string& endpoint) {
  grpc::ChannelArguments args;
  // The engine returns whole graphs and source location collections; the default 4 MB receive limit
  // is reached by real projects. The engine server lifts the same limits.
  args.SetMaxReceiveMessageSize(-1);
  args.SetMaxSendMessageSize(-1);
  return grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), args);
}

}    // namespace

EngineChannel::EngineChannel(std::string endpoint) : mEndpoint(std::move(endpoint)) {
  mChannel = createChannel(mEndpoint);
  mStub = sourcetrail::EngineService::NewStub(mChannel);
}

const std::string& EngineChannel::getEndpoint() const {
  return mEndpoint;
}

void EngineChannel::reconnect(std::string endpoint) {
  mEndpoint = std::move(endpoint);
  mChannel = createChannel(mEndpoint);
  mStub = sourcetrail::EngineService::NewStub(mChannel);
  setConnected(false);
}

bool EngineChannel::waitUntilReady(std::chrono::milliseconds timeout) {
  if(!mChannel) {
    return false;
  }

  const auto deadline = std::chrono::system_clock::now() + timeout;
  // try_to_connect: without it an idle channel never leaves IDLE and the wait always times out.
  const bool ready = mChannel->WaitForConnected(deadline);
  setConnected(ready);
  return ready;
}

sourcetrail::EngineService::Stub* EngineChannel::getStub() const {
  return mStub.get();
}

bool EngineChannel::isConnected() const {
  return mConnected;
}

void EngineChannel::markConnected() {
  setConnected(true);
}

void EngineChannel::markDegraded() {
  setConnected(false);
}

void EngineChannel::setConnectionStateHandler(std::function<void(bool)> handler) {
  mConnectionStateHandler = std::move(handler);
}

std::chrono::milliseconds EngineChannel::getCallTimeout() const {
  return mCallTimeout;
}

void EngineChannel::setCallTimeout(std::chrono::milliseconds timeout) {
  mCallTimeout = timeout;
}

void EngineChannel::setConnected(bool connected) {
  if(mConnected == connected) {
    return;
  }

  mConnected = connected;
  LOG_INFO(connected ? "Engine connection established: " + mEndpoint : "Engine connection lost: " + mEndpoint);

  if(mConnectionStateHandler) {
    mConnectionStateHandler(connected);
  }
}
