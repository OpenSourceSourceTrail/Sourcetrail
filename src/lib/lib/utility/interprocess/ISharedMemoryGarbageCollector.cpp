#include "utility/interprocess/ISharedMemoryGarbageCollector.hpp"

#include <cassert>

namespace lib {

ISharedMemoryGarbageCollector::Ptr ISharedMemoryGarbageCollector::sInstance;

void ISharedMemoryGarbageCollector::setInstance(Ptr instance) noexcept {
  sInstance = std::move(instance);
}

ISharedMemoryGarbageCollector::Ptr ISharedMemoryGarbageCollector::getInstance() noexcept {
  assert(sInstance);
  return sInstance;
}

// No assert here, unlike getInstance(): a null collector is a supported state, not a bug. Every
// caller -- SharedMemory::SharedMemory/~SharedMemory and Application::~Application -- guards with
// `if(auto* collector = getInstanceRaw(); collector)`, and a headless run never installs one at all.
ISharedMemoryGarbageCollector::RawPtr ISharedMemoryGarbageCollector::getInstanceRaw() noexcept {
  return sInstance.get();
}

ISharedMemoryGarbageCollector::ISharedMemoryGarbageCollector() noexcept = default;
ISharedMemoryGarbageCollector::~ISharedMemoryGarbageCollector() noexcept = default;

}    // namespace lib