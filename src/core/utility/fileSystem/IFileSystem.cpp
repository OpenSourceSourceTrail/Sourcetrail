#include "IFileSystem.hpp"

namespace core::utility::filesystem {

IFileSystem::Ptr IFileSystem::sInstance;

IFileSystem::Ptr IFileSystem::instance() noexcept {
  assert(sInstance);
  return sInstance;
}

IFileSystem::Raw IFileSystem::getInstanceRaw() noexcept {
  assert(sInstance);
  return sInstance.get();
}

void IFileSystem::setInstance(Ptr instance) noexcept {
  sInstance = std::move(instance);
}

IFileSystem::~IFileSystem() noexcept = default;

}    // namespace core::utility::filesystem
