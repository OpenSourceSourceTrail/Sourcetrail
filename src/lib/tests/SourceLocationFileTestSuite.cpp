// GTest
#include <gmock/gmock.h>
#include <gtest/gtest.h>
// internal
#include "SourceLocationFile.h"

using namespace std::string_literals;
using namespace testing;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp,cppcoreguidelines-owning-memory,readability-function-cognitive-complexity)
TEST(SourceLocationFile, constractor) {
  const auto* const language = L"cpp";
  const auto filePath = FilePath{"main.cpp"};
  const SourceLocationFile sourceLocationFile(filePath, language, true, true, true);
  EXPECT_EQ(sourceLocationFile.getFilePath(), filePath);
  EXPECT_THAT(sourceLocationFile.getLanguage(), StrEq(language));
  EXPECT_TRUE(sourceLocationFile.isWhole());
  EXPECT_TRUE(sourceLocationFile.isComplete());
  EXPECT_TRUE(sourceLocationFile.isIndexed());

  EXPECT_EQ(sourceLocationFile.getSourceLocationCount(), 0);
  EXPECT_EQ(sourceLocationFile.getUnscopedStartLocationCount(), 0);
}

class SourceLocationFileFix : public testing::Test {
public:
  void SetUp() override {
    const auto* const language = L"cpp";
    const auto filePath = FilePath{"main.cpp"};
    m_pSourceLocationFile = std::make_unique<SourceLocationFile>(filePath, language, true, true, true);
  }

  std::unique_ptr<SourceLocationFile> m_pSourceLocationFile;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp,cppcoreguidelines-owning-memory)
TEST_F(SourceLocationFileFix, SourceLocation) {
  const auto* const pSourceLocation = m_pSourceLocationFile->addSourceLocation(
      LocationType::LOCATION_FULLTEXT_SEARCH, Id{1}, std::vector<Id>{}, 1, 1, 2, 2);
  EXPECT_NE(pSourceLocation, nullptr);
  EXPECT_EQ(m_pSourceLocationFile->getUnscopedStartLocationCount(), 1);
  EXPECT_EQ(m_pSourceLocationFile->getSourceLocationCount(), 1);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp,cppcoreguidelines-owning-memory)
TEST_F(SourceLocationFileFix, SourceLocationWithZeroLocationId) {
  const auto* const pSourceLocation = m_pSourceLocationFile->addSourceLocation(
      LocationType::LOCATION_FULLTEXT_SEARCH, Id{0}, std::vector<Id>{}, 1, 1, 2, 2);
  EXPECT_NE(pSourceLocation, nullptr);
  EXPECT_EQ(m_pSourceLocationFile->getUnscopedStartLocationCount(), 1);
  EXPECT_EQ(m_pSourceLocationFile->getSourceLocationCount(), 0);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp,cppcoreguidelines-owning-memory)
TEST_F(SourceLocationFileFix, SourceLocationCopy) {
  // NOTE: Passing nullptr or m_pSourceLocationFile.get() make no difference
  const SourceLocation sourceLocation{nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, Id{2}, std::vector<Id>{}, 1, 1, true};

  const auto* const pSourceLocation = m_pSourceLocationFile->addSourceLocationCopy(&sourceLocation);
  EXPECT_NE(pSourceLocation, nullptr);
  EXPECT_EQ(m_pSourceLocationFile->getUnscopedStartLocationCount(), 1);
  EXPECT_EQ(m_pSourceLocationFile->getSourceLocationCount(), 1);
}