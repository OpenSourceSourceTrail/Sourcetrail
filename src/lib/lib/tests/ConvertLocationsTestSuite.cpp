#include <gtest/gtest.h>

#include "ConvertLocations.h"
#include "SourceLocation.h"
#include "SourceLocationCollection.h"
#include "SourceLocationFile.h"

using namespace proto::convert;

namespace {

std::shared_ptr<SourceLocationFile> makeFile() {
  return std::make_shared<SourceLocationFile>(FilePath(L"/src/main.cpp"), L"cpp", false, true, true);
}

}    // namespace

TEST(ConvertLocations, fileMetadataRoundTrips) {
  const SourceLocationFile file(FilePath(L"/src/other.cpp"), L"java", true, false, true);

  const auto restored = fromProto(toProto(file));

  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->getFilePath().wstr(), L"/src/other.cpp");
  EXPECT_EQ(restored->getLanguage(), L"java");
  EXPECT_TRUE(restored->isWhole());
  EXPECT_FALSE(restored->isComplete());
  EXPECT_TRUE(restored->isIndexed());
  EXPECT_EQ(restored->getSourceLocationCount(), 0U);
}

TEST(ConvertLocations, singleLocationRoundTrips) {
  const auto file = makeFile();
  file->addSourceLocation(LOCATION_TOKEN, 42, {7, 8}, 3, 5, 3, 11);

  const auto restored = fromProto(toProto(*file));

  // getSourceLocationCount() counts logical locations (the id index), not the start/end pair.
  ASSERT_EQ(restored->getSourceLocationCount(), 1U);

  const SourceLocation* start = restored->getSourceLocationById(42);
  ASSERT_NE(start, nullptr);
  EXPECT_TRUE(start->isStartLocation());
  EXPECT_EQ(start->getType(), LOCATION_TOKEN);
  EXPECT_EQ(start->getTokenIds(), (std::vector<Id>{7, 8}));
  EXPECT_EQ(start->getLineNumber(), 3U);
  EXPECT_EQ(start->getColumnNumber(), 5U);

  const SourceLocation* end = start->getEndLocation();
  ASSERT_NE(end, nullptr);
  EXPECT_EQ(end->getLineNumber(), 3U);
  EXPECT_EQ(end->getColumnNumber(), 11U);
}

TEST(ConvertLocations, multiLineLocationRoundTrips) {
  const auto file = makeFile();
  file->addSourceLocation(LOCATION_SCOPE, 1, {100}, 10, 1, 25, 2);

  const auto restored = fromProto(toProto(*file));

  const SourceLocation* start = restored->getSourceLocationById(1);
  ASSERT_NE(start, nullptr);
  EXPECT_EQ(start->getType(), LOCATION_SCOPE);
  EXPECT_EQ(start->getLineNumber(), 10U);
  EXPECT_EQ(start->getEndLocation()->getLineNumber(), 25U);
  EXPECT_EQ(start->getEndLocation()->getColumnNumber(), 2U);
}

TEST(ConvertLocations, locationTypesRoundTrip) {
  const auto file = makeFile();
  file->addSourceLocation(LOCATION_TOKEN, 1, {1}, 1, 1, 1, 2);
  file->addSourceLocation(LOCATION_SCOPE, 2, {2}, 2, 1, 2, 2);
  file->addSourceLocation(LOCATION_QUALIFIER, 3, {3}, 3, 1, 3, 2);
  file->addSourceLocation(LOCATION_LOCAL_SYMBOL, 4, {4}, 4, 1, 4, 2);
  file->addSourceLocation(LOCATION_SIGNATURE, 5, {5}, 5, 1, 5, 2);
  file->addSourceLocation(LOCATION_COMMENT, 6, {6}, 6, 1, 6, 2);
  file->addSourceLocation(LOCATION_ERROR, 7, {7}, 7, 1, 7, 2);
  file->addSourceLocation(LOCATION_FULLTEXT_SEARCH, 8, {8}, 8, 1, 8, 2);

  const auto restored = fromProto(toProto(*file));

  EXPECT_EQ(restored->getSourceLocationById(1)->getType(), LOCATION_TOKEN);
  EXPECT_EQ(restored->getSourceLocationById(2)->getType(), LOCATION_SCOPE);
  EXPECT_EQ(restored->getSourceLocationById(3)->getType(), LOCATION_QUALIFIER);
  EXPECT_EQ(restored->getSourceLocationById(4)->getType(), LOCATION_LOCAL_SYMBOL);
  EXPECT_EQ(restored->getSourceLocationById(5)->getType(), LOCATION_SIGNATURE);
  EXPECT_EQ(restored->getSourceLocationById(6)->getType(), LOCATION_COMMENT);
  EXPECT_EQ(restored->getSourceLocationById(7)->getType(), LOCATION_ERROR);
  EXPECT_EQ(restored->getSourceLocationById(8)->getType(), LOCATION_FULLTEXT_SEARCH);
}

TEST(ConvertLocations, locationWithoutTokenIdsRoundTrips) {
  const auto file = makeFile();
  file->addSourceLocation(LOCATION_FULLTEXT_SEARCH, 9, {}, 1, 1, 1, 4);

  const auto restored = fromProto(toProto(*file));

  const SourceLocation* start = restored->getSourceLocationById(9);
  ASSERT_NE(start, nullptr);
  EXPECT_TRUE(start->getTokenIds().empty());
}

TEST(ConvertLocations, emptyCollectionRoundTrips) {
  const SourceLocationCollection collection;

  const auto restored = fromProto(toProto(collection));

  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->getSourceLocationFileCount(), 0U);
}

TEST(ConvertLocations, collectionWithMultipleFilesRoundTrips) {
  SourceLocationCollection collection;
  collection.addSourceLocation(LOCATION_TOKEN, 1, {10}, FilePath(L"/src/a.cpp"), 1, 1, 1, 5);
  collection.addSourceLocation(LOCATION_TOKEN, 2, {20}, FilePath(L"/src/b.cpp"), 2, 3, 2, 9);
  collection.addSourceLocation(LOCATION_SCOPE, 3, {30}, FilePath(L"/src/b.cpp"), 5, 1, 8, 1);

  const auto restored = fromProto(toProto(collection));

  ASSERT_EQ(restored->getSourceLocationFileCount(), 2U);

  const auto fileA = restored->getSourceLocationFileByPath(FilePath(L"/src/a.cpp"));
  ASSERT_NE(fileA, nullptr);
  EXPECT_EQ(fileA->getSourceLocationCount(), 1U);

  const auto fileB = restored->getSourceLocationFileByPath(FilePath(L"/src/b.cpp"));
  ASSERT_NE(fileB, nullptr);
  EXPECT_EQ(fileB->getSourceLocationCount(), 2U);

  const SourceLocation* scope = restored->getSourceLocationById(3);
  ASSERT_NE(scope, nullptr);
  EXPECT_EQ(scope->getType(), LOCATION_SCOPE);
  EXPECT_EQ(scope->getEndLocation()->getLineNumber(), 8U);
}
