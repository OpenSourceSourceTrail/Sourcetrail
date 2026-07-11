#include "ISharedMemoryGarbageCollector.hpp"

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

ISharedMemoryGarbageCollector::RawPtr ISharedMemoryGarbageCollector::getInstanceRaw() noexcept {
  assert(sInstance);
  return sInstance.get();
}

ISharedMemoryGarbageCollector::ISharedMemoryGarbageCollector() noexcept = default;
ISharedMemoryGarbageCollector::~ISharedMemoryGarbageCollector() noexcept = default;

}    // namespace lib