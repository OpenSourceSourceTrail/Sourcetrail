#include "EngineHost.h"

#include <iostream>

#include <fmt/core.h>

#include "app/IndexerPluginRegistry.h"
#include "EngineEventPublisher.h"
#include "EngineHttpService.h"
#include "HttpServer.h"
#include "project/CxxToolchainRemote.h"
#include "project/ICxxToolchain.h"
#include "project/SourceGroupFactory.h"
#include "project/SourceGroupFactoryModuleCustom.h"
#include "project/SourceGroupFactoryModuleCxx.h"
#include "project/SourceGroupFactoryModuleJava.h"
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
    , mPublisher(std::make_unique<EngineEventPublisher>(mService.get())) {
  if(!broadcastOnly) {
    mPublisher->installDialogViewFactory();
  }
}

HttpEndpoint::~HttpEndpoint() {
  stop();
}

uint16_t HttpEndpoint::start(uint16_t port) {
  // A loopback HTTP port is reachable by every process on the machine, and by any page the user's
  // browser loads -- which the gRPC port it replaces was not. The token is the proof of being the
  // client this engine was started for; it is handed over on the handshake line below, which only
  // the parent process can read.
  mServer = std::make_unique<http::Server>(utility::getUuidString());
  mService->registerRoutes(*mServer);

  const uint16_t assignedPort = mServer->start(port);
  if(assignedPort == 0) {
    mServer.reset();
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
    mServer->stop();
    mServer.reset();
  }
}

EngineHttpService& HttpEndpoint::service() const {
  return *mService;
}

}    // namespace engine_host
