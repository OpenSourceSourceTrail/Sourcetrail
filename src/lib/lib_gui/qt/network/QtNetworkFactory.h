#pragma once

#include "component/NetworkFactory.h"

class QtNetworkFactory final : public NetworkFactory {
public:
  ~QtNetworkFactory() override;

  std::shared_ptr<IDECommunicationController> createIDECommunicationController(StorageAccess* storageAccess) const override;
};
