#pragma once

#include "ide_communication/logic/NetworkFactory.h"

class QtNetworkFactory final : public NetworkFactory {
public:
  ~QtNetworkFactory() override;

  std::shared_ptr<IDECommunicationController> createIDECommunicationController(StorageAccess* storageAccess) const override;
};
