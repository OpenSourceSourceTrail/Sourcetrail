#pragma once

#include <memory>

namespace lib {

struct ISharedMemoryGarbageCollector {
  using Ptr = std::shared_ptr<ISharedMemoryGarbageCollector>;
  using RawPtr = ISharedMemoryGarbageCollector*;

  static void setInstance(Ptr instance) noexcept;

  static Ptr getInstance() noexcept;

  static RawPtr getInstanceRaw() noexcept;

  ISharedMemoryGarbageCollector() noexcept;
  virtual ~ISharedMemoryGarbageCollector() noexcept;

  virtual void run(const std::string& uuid) noexcept = 0;
  virtual void stop() noexcept = 0;

  virtual void registerSharedMemory(const std::string& sharedMemoryName) noexcept = 0;
  virtual void unregisterSharedMemory(const std::string& sharedMemoryName) noexcept = 0;

private:
  static Ptr sInstance;
};

}    // namespace lib