#include "network/QtNetworkFactory.h"

#include "network/QtIDECommunicationController.h"

QtNetworkFactory::~QtNetworkFactory() = default;

std::shared_ptr<IDECommunicationController> QtNetworkFactory::createIDECommunicationController(StorageAccess* storageAccess) const {
  return std::make_shared<QtIDECommunicationController>(nullptr, storageAccess);
}
