#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IndexerComposite.h"
#include "LanguagePackage.h"
#include "LanguagePackageManager.h"
#include "mocks/MockedIndexer.hpp"
#include "ScopedFunctor.h"

using namespace testing;

struct MockedLanguagePackage : public LanguagePackage {
  ~MockedLanguagePackage() override = default;
  using VecOfIndexer = std::vector<std::shared_ptr<IndexerBase>>;
  MOCK_METHOD(VecOfIndexer, instantiateSupportedIndexers, (), (const, override));
};

TEST(LanguagePackageManager, instance) {
  auto instance = LanguagePackageManager::getInstance();
  ASSERT_EQ(2, instance.use_count());
  LanguagePackageManager::destroyInstance();
  ASSERT_EQ(1, instance.use_count());
}

TEST(LanguagePackageManager, emptyIndexers) {
  auto instance = LanguagePackageManager::getInstance();
  [[maybe_unused]] ScopedFunctor func([]() { LanguagePackageManager::destroyInstance(); });

  auto mockedPackage = std::make_shared<MockedLanguagePackage>();
  EXPECT_CALL(*mockedPackage, instantiateSupportedIndexers).WillOnce(Return(std::vector<std::shared_ptr<IndexerBase>>{}));
  instance->addPackage(mockedPackage);
  auto indexers = instance->instantiateSupportedIndexers();
}

#if BUILD_CXX_LANGUAGE_PACKAGE
TEST(LanguagePackageManager, goodCase) {
  auto instance = LanguagePackageManager::getInstance();
  [[maybe_unused]] ScopedFunctor func([]() { LanguagePackageManager::destroyInstance(); });

  auto mockedPackage = std::make_shared<MockedLanguagePackage>();
  auto mockedIndexer = std::make_shared<MockedIndexer>();
  EXPECT_CALL(*mockedPackage, instantiateSupportedIndexers).WillOnce(Return(std::vector<std::shared_ptr<IndexerBase>>{mockedIndexer}));
  EXPECT_CALL(*mockedIndexer, getSupportedIndexerCommandType).WillOnce(Return(INDEXER_COMMAND_CXX));
  instance->addPackage(mockedPackage);
  auto indexers = instance->instantiateSupportedIndexers();
}
#endif