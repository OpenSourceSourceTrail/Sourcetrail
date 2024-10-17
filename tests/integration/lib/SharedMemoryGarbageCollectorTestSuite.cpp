#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gmock/gmock.h"
#include "SharedMemoryGarbageCollector.h"

struct MockedSharedMemory : ISharedMemory {};

struct SharedMemoryGarbageCollectorFix : testing::Test {
  void SetUp() override {
    mGC = std::make_unique<SharedMemoryGarbageCollector>(nullptr);
  }
  testing::StrictMock<MockedSharedMemory>* mSharedMemory;
  std::unique_ptr<SharedMemoryGarbageCollector> mGC;
};

TEST_F(SharedMemoryGarbageCollectorFix, goodCase) {
  // GIVEN:
  // WHEN:
  // THEN:
}
