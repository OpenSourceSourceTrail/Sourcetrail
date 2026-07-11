// GTest
#include <istream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
// internal
#include "FilePath.h"
#include "SourceLocation.h"
#include "SourceLocationFile.h"

using namespace ::testing;
using namespace std::string_literals;

// NOLINTNEXTLINE
TEST(SourceLocation, createStartSourceLocation) {
  const SourceLocation sourceLocation(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{}, 1, 1, true);
  EXPECT_EQ(sourceLocation.getSourceLocationFile(), nullptr);
  EXPECT_EQ(sourceLocation.getStartLocation(), &sourceLocation);
  EXPECT_EQ(sourceLocation.getEndLocation(), nullptr);
  EXPECT_EQ(sourceLocation.getOtherLocation(), nullptr);
  EXPECT_EQ(sourceLocation.getLocationId(), 1);
  EXPECT_THAT(sourceLocation.getTokenIds(), IsEmpty());
  EXPECT_EQ(sourceLocation.getType(), LocationType::LOCATION_FULLTEXT_SEARCH);
  EXPECT_TRUE(sourceLocation.isStartLocation());
  EXPECT_FALSE(sourceLocation.isEndLocation());
  EXPECT_TRUE(sourceLocation.isFullTextSearchMatch());

  EXPECT_EQ(1, sourceLocation.getLineNumber());
  EXPECT_EQ(1, sourceLocation.getColumnNumber());

  EXPECT_EQ(FilePath::EmptyFilePath(), sourceLocation.getFilePath());
}

// NOLINTNEXTLINE
TEST(SourceLocation, createEndSourceLocation) {
  SourceLocation startSourceLocation(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{}, 1, 1, true);

  const SourceLocationFile sourceFile{FilePath{"main.cpp"}, L".cpp", true, true, true};
  const auto endSourceLocation = SourceLocation::createEndSourceLocation(&startSourceLocation, 1, 2);
  EXPECT_EQ(endSourceLocation->getSourceLocationFile(), nullptr);
  EXPECT_EQ(endSourceLocation->getStartLocation(), &startSourceLocation);
  EXPECT_EQ(endSourceLocation->getEndLocation(), endSourceLocation.get());
  EXPECT_EQ(endSourceLocation->getOtherLocation(), &startSourceLocation);
  EXPECT_EQ(endSourceLocation->getLocationId(), 1);
  EXPECT_THAT(endSourceLocation->getTokenIds(), IsEmpty());
  EXPECT_EQ(endSourceLocation->getType(), LocationType::LOCATION_FULLTEXT_SEARCH);
  EXPECT_FALSE(endSourceLocation->isStartLocation());
  EXPECT_TRUE(endSourceLocation->isEndLocation());
}

// NOLINTNEXTLINE
TEST(SourceLocation, createEndSourceLocationSameAsStart) {
  // NOTE(Hussein): Should shaw some warning?
  SourceLocation startSourceLocation(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{}, 1, 1, true);

  const SourceLocationFile sourceFile{FilePath{"main.cpp"}, L".cpp", true, true, true};
  const auto endSourceLocation = SourceLocation::createEndSourceLocation(&startSourceLocation, 1, 1);
  EXPECT_EQ(endSourceLocation->getSourceLocationFile(), nullptr);
  EXPECT_EQ(endSourceLocation->getStartLocation(), &startSourceLocation);
  EXPECT_EQ(endSourceLocation->getEndLocation(), endSourceLocation.get());
  EXPECT_EQ(endSourceLocation->getOtherLocation(), &startSourceLocation);
  EXPECT_EQ(endSourceLocation->getLocationId(), 1);
  EXPECT_THAT(endSourceLocation->getTokenIds(), IsEmpty());
  EXPECT_EQ(endSourceLocation->getType(), LocationType::LOCATION_FULLTEXT_SEARCH);
  EXPECT_FALSE(endSourceLocation->isStartLocation());
  EXPECT_TRUE(endSourceLocation->isEndLocation());
}

// NOLINTNEXTLINE
TEST(SourceLocation, createSourceLocationFromAnother) {
  SourceLocation sourceLocation(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{}, 1, 1, true);

  SourceLocationFile sourceFile{FilePath{"main.cpp"}, L".cpp", true, true, true};
  auto newStartSourceLocation = SourceLocation::createSourceLocationFromAnother(&sourceLocation, &sourceFile);
  EXPECT_EQ(newStartSourceLocation->getSourceLocationFile(), &sourceFile);
  EXPECT_EQ(newStartSourceLocation->getStartLocation(), newStartSourceLocation.get());
  EXPECT_EQ(newStartSourceLocation->getEndLocation(), nullptr);
  EXPECT_EQ(newStartSourceLocation->getOtherLocation(), nullptr);
  EXPECT_EQ(newStartSourceLocation->getLocationId(), 1);
  EXPECT_THAT(newStartSourceLocation->getTokenIds(), IsEmpty());
  EXPECT_EQ(newStartSourceLocation->getType(), LocationType::LOCATION_FULLTEXT_SEARCH);
  EXPECT_TRUE(newStartSourceLocation->isStartLocation());
  EXPECT_FALSE(newStartSourceLocation->isEndLocation());
}

// NOLINTNEXTLINE
TEST(SourceLocation, serializeStartSourceLocation) {
  const SourceLocation sourceLocation(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{}, 1, 1, true);
  std::wstringstream wStream;
  wStream << sourceLocation;

  std::wstring result = L"<1:[ \b] ";
  EXPECT_THAT(wStream.str(), Eq(result));
}

// NOLINTNEXTLINE
TEST(SourceLocation, serializeEndSourceLocation) {
  SourceLocation startSourceLocation(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{2, 3, 4}, 1, 1, true);

  const SourceLocationFile sourceFile{FilePath{"main.cpp"}, L".cpp", true, true, true};
  const auto endSourceLocation = SourceLocation::createEndSourceLocation(&startSourceLocation, 1, 2);

  std::wstringstream wStream;
  wStream << startSourceLocation;

  std::wstring result = L"<1:[ \b2 \b3 \b4 \b] ";
  EXPECT_THAT(wStream.str(), Eq(result));

  // Clear stream buffer
  wStream.str(L"");

  wStream << *endSourceLocation;

  result = L"2:[ \b2 \b3 \b4 \b]> ";
  EXPECT_THAT(wStream.str(), Eq(result));
}

// NOLINTNEXTLINE
TEST(SourceLocation, equalOperator) {
  SourceLocation sourceLocation0(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{}, 1, 1, true);

  SourceLocation sourceLocation1(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{}, 1, 1, true);

  EXPECT_EQ(sourceLocation0, sourceLocation1);

  SourceLocation sourceLocation2(nullptr, LocationType::LOCATION_FULLTEXT_SEARCH, 1, std::vector<Id>{}, 1, 2, true);

  EXPECT_FALSE(sourceLocation0 == sourceLocation2);
}