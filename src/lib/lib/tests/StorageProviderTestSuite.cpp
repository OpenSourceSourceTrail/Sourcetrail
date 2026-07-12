#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "StorageProvider.h"

TEST(StorageProvider, getStorageCount_empty) {
  // Given:
  const StorageProvider provider;
  // When:
  const auto result = provider.getStorageCount();
  // Then:
  EXPECT_EQ(0, result);
}

TEST(StorageProvider, getStorageCount_afterClear) {
  // Given:
  StorageProvider provider;
  // And:
  provider.clear();
  // When:
  const auto result = provider.getStorageCount();
  // Then:
  EXPECT_EQ(0, result);
}

TEST(StorageProvider, insert_nullptr) {
  // Given:
  StorageProvider provider;
  // When:
  const auto result = provider.insert(nullptr);
  // Then:
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Storage is null");
}

TEST(StorageProvider, insert_multipleStorages) {
  // Given:
  StorageProvider provider;
  // When:
  provider.insert(std::make_shared<IntermediateStorage>());
  provider.insert(std::make_shared<IntermediateStorage>());
  provider.insert(std::make_shared<IntermediateStorage>());
  // And:
  const auto result = provider.getStorageCount();
  // Then:
  EXPECT_EQ(3, result);
}

TEST(StorageProvider, consumeSecondLargestStorage_empty) {
  // Given:
  StorageProvider provider;
  // When:
  const auto result = provider.consumeSecondLargestStorage();
  // Then:
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "No Storage found");
}

TEST(StorageProvider, consumeSecondLargestStorage_oneStorageExists) {
  // Given:
  StorageProvider provider;
  // And:
  provider.insert(std::make_shared<IntermediateStorage>());
  // When:
  const auto result = provider.consumeSecondLargestStorage();
  // Then:
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "No Storage found");
}

TEST(StorageProvider, consumeSecondLargestStorage_multiStorageExists) {
  // Given:
  StorageProvider provider;
  // And:
  provider.insert(std::make_shared<IntermediateStorage>());
  provider.insert(std::make_shared<IntermediateStorage>());
  provider.insert(std::make_shared<IntermediateStorage>());
  // When:
  const auto result = provider.consumeSecondLargestStorage();
  // Then:
  ASSERT_TRUE(result.has_value());
}

TEST(StorageProvider, consumeLargestStorage_empty) {
  // Given:
  StorageProvider provider;
  // When:
  const auto result = provider.consumeLargestStorage();
  // Then:
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "No Storage found");
}

TEST(StorageProvider, consumeLargestStorage_oneStorageExists) {
  // Given:
  StorageProvider provider;
  // And:
  provider.insert(std::make_shared<IntermediateStorage>());
  // When:
  const auto result = provider.consumeLargestStorage();
  // Then:
  EXPECT_TRUE(result.has_value());
}
