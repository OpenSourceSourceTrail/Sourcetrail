#include <memory>
#include <thread>

#include <gtest/gtest.h>

#include "ISharedMemoryGarbageCollector.hpp"
#include "MockedSharedMemoryGarbageCollector.hpp"
#include "SharedMemory.h"

using namespace testing;

struct SharedMemoryFixture : testing::Test {
  void SetUp() override {
    mSharedMemoryGarbageCollector = std::make_shared<lib::MockedSharedMemoryGarbageCollector>();
    lib::ISharedMemoryGarbageCollector::setInstance(mSharedMemoryGarbageCollector);
  }

  void TearDown() override {
    lib::ISharedMemoryGarbageCollector::setInstance(nullptr);
    mSharedMemoryGarbageCollector.reset();
  }

  lib::MockedSharedMemoryGarbageCollector::Ptr mSharedMemoryGarbageCollector;
};

TEST_F(SharedMemoryFixture, goodCase) {
  EXPECT_CALL(*mSharedMemoryGarbageCollector, registerSharedMemory).WillOnce(Return());
  EXPECT_CALL(*mSharedMemoryGarbageCollector, unregisterSharedMemory).WillOnce(Return());

  SharedMemory memory("memory", 1000, SharedMemory::CREATE_AND_DELETE);

  {
    SharedMemory::ScopedAccess access(&memory);

    EXPECT_TRUE(access.getMemorySize() == 1000);
    *access.accessValue<int>("count") = 0;
  }

  std::vector<std::shared_ptr<std::thread>> threads;
  for(unsigned int i = 0; i < 4; i++) {
    threads.push_back(std::make_shared<std::thread>([]() {
      SharedMemory memory_("memory", 0, SharedMemory::OPEN_ONLY);

      SharedMemory::ScopedAccess access(&memory_);

      if(access.getMemorySize() < 5000) {
        access.growMemory(5000 - access.getMemorySize());
      }

      int* count = access.accessValue<int>("count");
      *count += 1;

      auto* str = access.accessValueWithAllocator<SharedMemory::String>("string");
      str->append("hi");

      auto* nums = access.accessValueWithAllocator<SharedMemory::Vector<int>>("nums");
      nums->push_back(static_cast<int>(nums->size()));

      auto* strings = access.accessValueWithAllocator<SharedMemory::Vector<SharedMemory::String>>("strings");
      strings->push_back(SharedMemory::String("ho", access.getAllocator()));

      auto* vals = access.accessValueWithAllocator<SharedMemory::Map<int, int>>("vals");
      vals->emplace(static_cast<int>(vals->size()), static_cast<int>(vals->size() * vals->size()));
    }));
  }

  for(auto& thread : threads) {
    thread->join();
  }
  threads.clear();

  {
    SharedMemory::ScopedAccess access(&memory);

    EXPECT_EQ(5000, access.getMemorySize());
    EXPECT_EQ(4, *access.accessValue<int>("count"));
    const std::string value = access.accessValueWithAllocator<SharedMemory::String>("string")->c_str();
    EXPECT_TRUE(value == "hihihihi");

    auto* nums = access.accessValueWithAllocator<SharedMemory::Vector<int>>("nums");
    EXPECT_EQ(4, nums->size());
    for(int i : *nums) {
      EXPECT_TRUE(i == i);
    }

    auto* strings = access.accessValueWithAllocator<SharedMemory::Vector<SharedMemory::String>>("strings");
    EXPECT_EQ(4, strings->size());
    for(SharedMemory::String& str : *strings) {
      EXPECT_TRUE(str == "ho");
    }

    auto* vals = access.accessValueWithAllocator<SharedMemory::Map<int, int>>("vals");
    EXPECT_EQ(4, vals->size());

    size_t i = 0;
    for(auto val : *vals) {
      EXPECT_EQ(i, val.first);
      EXPECT_EQ(i * i, val.second);
      i++;
    }
  }
}
