#include "ITaskManager.hpp"

#include <cassert>

namespace scheduling {

ITaskManager::Ptr ITaskManager::sInstance;

void ITaskManager::setInstance(Ptr instance) noexcept {
  sInstance = std::move(instance);
}

ITaskManager::Ptr ITaskManager::getInstance() noexcept {
  assert(sInstance);
  return sInstance;
}

ITaskManager::RawPtr ITaskManager::getInstanceRaw() noexcept {
  assert(sInstance);
  return sInstance.get();
}

ITaskManager::~ITaskManager() noexcept = default;

}    // namespace lib