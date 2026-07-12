#pragma once

#include <memory>

class IDECommunicationController;
class StorageAccess;

class NetworkFactory {
public:
  virtual ~NetworkFactory();

  virtual std::shared_ptr<IDECommunicationController> createIDECommunicationController(StorageAccess* storageAccess) const = 0;
};
