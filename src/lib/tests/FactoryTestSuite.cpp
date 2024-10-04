#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "impls/Factory.hpp"

TEST(FactoryTestSuite, goodCase) {
  lib::Factory factory;
  EXPECT_THAT(factory.createProject(nullptr, nullptr, "", false), testing::NotNull());
  EXPECT_THAT(factory.createMessageQueue(), testing::NotNull());
  EXPECT_THAT(factory.createSharedMemoryGarbageCollector(), testing::NotNull());
  EXPECT_THAT(factory.createTaskManager(), testing::NotNull());
}
