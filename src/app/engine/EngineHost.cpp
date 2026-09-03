#include "EngineHost.h"

#include <iostream>

#include <fmt/core.h>

#include "app/IndexerPluginRegistry.h"
#include "EngineEventPublisher.h"
#include "EngineHttpService.h"
#include "HttpServer.h"
#include "project/logic/CxxToolchainRemote.h"
#include "project/logic/ICxxToolchain.h"
#include "project/logic/SourceGroupFactory.h"
#include "project/logic/SourceGroupFactoryModuleCustom.h"
#include "project/logic/SourceGroupFactoryModuleCxx.h"
#include "project/logic/SourceGroupFactoryModuleJava.h"
#include "utilityUuid.h"

namespace engine_host {

void registerSourceGroupModules() {
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCustom>());
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleJava>());
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCxx>());
  ICxxToolchain::setInstance(std::make_shared<CxxToolchainRemote>());

  IndexerPluginRegistry::getInstance()->discover();
}

HttpEndpoint::HttpEndpoint(StorageAccess* storageAccess, bool broadcastOnly)
    : mService(std::make_unique<EngineHttpService>(storageAccess))
    // A loopback HTTP port is reachable by every process on the machine, and by any page the user's
    // browser loads -- which the gRPC port it replaces was not. The token is the proof of being the
    // client this engine was started for; start() hands it over on the handshake line, which only
    // the parent process can read. An in-process caller needs none of it, so the routes exist from
    // construction and callLocal() serves them whether or not anything ever listens.
    , mServer(std::make_unique<http::Server>(utility::getUuidString()))
    , mPublisher(std::make_unique<EngineEventPublisher>(mService.get())) {
  mService->registerRoutes(*mServer);
  if(!broadcastOnly) {
    mPublisher->installDialogViewFactory();
  }
}

std::optional<std::string> HttpEndpoint::callLocal(const std::string& method,
                                                   const std::string& target,
                                                   const std::string& body) const {
  const http::Response response = mServer->dispatch(method, target, body);
  if(response.status < 200 || response.status >= 300) {
    return std::nullopt;
  }
  return response.body;
}

HttpEndpoint::~HttpEndpoint() {
  stop();
}

uint16_t HttpEndpoint::start(uint16_t port) {
  const uint16_t assignedPort = mServer->start(port);
  if(assignedPort == 0) {
    return 0;
  }

  // Machine-readable handshake, emitted and flushed before anything else, so a parent that started
  // us with port 0 can learn the ephemeral port and the token.
  fmt::println("ENGINE_PORT {} {}", assignedPort, mServer->authToken());
  std::cout.flush();
  return assignedPort;
}

void HttpEndpoint::stop() {
  if(mService) {
    mService->abortDialogs();
  }
  if(mServer) {
    // Kept alive rather than reset: callLocal() answers from these routes, listener or not.
    mServer->stop();
  }
}

EngineHttpService& HttpEndpoint::service() const {
  return *mService;
}

}    // namespace engine_host
