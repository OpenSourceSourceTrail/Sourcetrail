#include <gmock/gmock.h>

#include "ISharedMemoryGarbageCollector.hpp"

namespace lib {
struct MockedSharedMemoryGarbageCollector : ISharedMemoryGarbageCollector {
  using Ptr = std::shared_ptr<lib::MockedSharedMemoryGarbageCollector>;

  MOCK_METHOD(void, run, (const std::string&), (noexcept, override));
  MOCK_METHOD(void, stop, (), (noexcept, override));

  MOCK_METHOD(void, registerSharedMemory, (const std::string&), (noexcept, override));
  MOCK_METHOD(void, unregisterSharedMemory, (const std::string&), (noexcept, override));
};
}    // namespace lib