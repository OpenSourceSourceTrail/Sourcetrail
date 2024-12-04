#include <range/v3/algorithm/copy.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FilePath.h"
#include "PersistentStorage.h"

namespace {

constexpr std::wstring_view kDbPath = L"data/test.sqlite";
constexpr std::wstring_view kBookmarkPath = L"data/testBookmarks.sqlite";

using testing::IsEmpty;
using testing::Test;

struct TaskClearStorageScenario : Test {
  void SetUp() override {
    mStorage = std::make_unique<PersistentStorage>(FilePath{kDbPath.data()}, FilePath{kBookmarkPath.data()});
    mStorage->setup();
  }

  void TearDown() override {}

  void createErrors() const {
    constexpr size_t ErrosCount = 10;
    for(Id index = 0; index < ErrosCount; ++index) {
      mStorage->addError({std::to_wstring(index), {}, false, false});
    }
  }

  std::unique_ptr<PersistentStorage> mStorage;
};

TEST_F(TaskClearStorageScenario, goodCase) {
  // Given:
  createErrors();
  // When:
  mStorage->clearAllErrors();
  // Then:
  ASSERT_THAT(mStorage->getErrors(), IsEmpty());
}

}    // namespace
