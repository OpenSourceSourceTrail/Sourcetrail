#pragma once

#include "NetworkFactory.h"

class QtNetworkFactory final : public NetworkFactory {
public:
  ~QtNetworkFactory() override;

  std::shared_ptr<IDECommunicationController> createIDECommunicationController(StorageAccess* storageAccess) const override;
};
