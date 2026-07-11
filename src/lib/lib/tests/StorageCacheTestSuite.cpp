#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Graph.h"
#include "TextAccess.h"
#define private public    // NOLINT(clang-diagnostic-keyword-macro)
#include "StorageCache.h"
#undef private

namespace {

TEST(StorageCache, getGraphForAll_empty) {
  const StorageCache cache;
  const auto graph = cache.getGraphForAll();
  ASSERT_THAT(cache.getGraphForAll(), testing::NotNull());
  EXPECT_EQ(0, graph->size());
}

TEST(StorageCache, getStorageStats_empty) {
  const StorageCache cache;
  const auto stats = cache.getStorageStats();
  EXPECT_EQ(0, stats.nodeCount);
}

TEST(StorageCache, getFileContent_empty) {
  StorageCache cache;
  cache.mUseErrorCache = true;
  const auto textAccess = cache.getFileContent(FilePath{}, true);
  ASSERT_THAT(textAccess, testing::NotNull());
  EXPECT_TRUE(textAccess->isEmpty());
}

TEST(StorageCache, getErrorCount_empty) {
  StorageCache cache;
  cache.mUseErrorCache = true;
  const auto errorsCount = cache.getErrorCount();
  EXPECT_EQ(0, errorsCount.total);
}

TEST(StorageCache, getErrorsLimited_empty) {
  StorageCache cache;
  cache.mUseErrorCache = true;
  const auto errorsInfo = cache.getErrorsLimited({});
  EXPECT_THAT(errorsInfo, testing::IsEmpty());
}

TEST(StorageCache, getErrorsForFileLimited_empty) {
  StorageCache cache;
  cache.mUseErrorCache = true;
  const auto errorsInfo = cache.getErrorsForFileLimited({}, FilePath{});
  EXPECT_THAT(errorsInfo, testing::IsEmpty());
}

TEST(StorageCache, getErrorSourceLocations_empty) {
  StorageCache cache;
  cache.mUseErrorCache = true;
  const auto sourceLocationCollection = cache.getErrorSourceLocations({});
  ASSERT_THAT(sourceLocationCollection, testing::NotNull());
}

TEST(StorageCache, setUseErrorCache_true) {
  StorageCache cache;
  cache.setUseErrorCache(true);

  EXPECT_TRUE(cache.mUseErrorCache);
  EXPECT_EQ(cache.mErrorCount.total, 0);
  EXPECT_EQ(cache.mErrorCount.fatal, 0);
  EXPECT_THAT(cache.mCachedErrors, testing::IsEmpty());
}

TEST(StorageCache, setUseErrorCache_false) {
  StorageCache cache;
  cache.setUseErrorCache(false);

  EXPECT_FALSE(cache.mUseErrorCache);
  EXPECT_EQ(cache.mErrorCount.total, 0);
  EXPECT_EQ(cache.mErrorCount.fatal, 0);
  EXPECT_THAT(cache.mCachedErrors, testing::IsEmpty());
}

TEST(StorageCache, addErrorsToCache) {
  StorageCache cache;
  cache.addErrorsToCache({}, {1, 2});
  EXPECT_EQ(cache.mErrorCount.total, 1);
  EXPECT_EQ(cache.mErrorCount.fatal, 2);
  EXPECT_THAT(cache.mCachedErrors, testing::IsEmpty());
}

}    // namespace
