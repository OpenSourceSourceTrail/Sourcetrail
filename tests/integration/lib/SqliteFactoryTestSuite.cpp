#include <filesystem>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "SqliteFactory.hpp"

namespace sqlite {

TEST(SqliteFactoryFix, CreateReadSqliteBookmarkStorage_EmptyPath) {
  SqliteFactory mFactory;
  EXPECT_THAT(mFactory.createReadSqliteBookmarkStorage({}), testing::IsNull());
}

TEST(SqliteFactoryFix, CreateReadSqliteBookmarkStorage_GoodCase) {
  SqliteFactory mFactory;
  ASSERT_THAT(mFactory.createReadSqliteBookmarkStorage("data/testBookmarks.sqlite"), testing::NotNull());
}

}    // namespace sqlite
