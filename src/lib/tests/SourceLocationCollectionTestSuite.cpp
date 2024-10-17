// GTest
#include <gmock/gmock.h>
#include <gtest/gtest.h>
// internal
#include "SourceLocation.h"
#include "SourceLocationCollection.h"
#include "SourceLocationFile.h"

class SourceLocationsGetCreatedWithOtherEnd : public testing::Test {
public:
  void SetUp() override {
    const uint32_t StartLocation[]{2, 3};
    const uint32_t EndLocation[]{4, 5};

    FilePath filePath(L"file.c");

    // clang-format off
    m_pStartLocation = mCollection.addSourceLocation(
        LOCATION_TOKEN, 1, {1},
        filePath,
        StartLocation[0], StartLocation[1],
        EndLocation[0], EndLocation[1]);
    // clang-format on
    ASSERT_NE(nullptr, m_pStartLocation);
  }

  SourceLocationCollection mCollection;
  SourceLocation* m_pStartLocation = nullptr;
};

// NOLINTNEXTLINE
TEST_F(SourceLocationsGetCreatedWithOtherEnd, goodCaseStartLocation) {
  EXPECT_TRUE(m_pStartLocation->isStartLocation());
  EXPECT_FALSE(m_pStartLocation->isEndLocation());
}

// NOLINTNEXTLINE
TEST_F(SourceLocationsGetCreatedWithOtherEnd, goodCaseEndLocation) {
  const auto* pEndLocation = m_pStartLocation->getOtherLocation();
  ASSERT_NE(nullptr, pEndLocation);

  EXPECT_FALSE(pEndLocation->isStartLocation());
  EXPECT_TRUE(pEndLocation->isEndLocation());

  EXPECT_EQ(m_pStartLocation, pEndLocation->getOtherLocation());
  EXPECT_EQ(m_pStartLocation, pEndLocation->getStartLocation());
  EXPECT_EQ(m_pStartLocation, m_pStartLocation->getStartLocation());

  EXPECT_EQ(pEndLocation, m_pStartLocation->getEndLocation());
  EXPECT_EQ(pEndLocation, pEndLocation->getEndLocation());
}

class SourceLocationsDoNotGetCreatedWithWrongInput : public testing::Test {
public:
  void SetUp() override {
    FilePath filePath(L"file.c");
    m_pLocation0 = collection.addSourceLocation(LOCATION_TOKEN, 1, {1}, filePath, 2, 3, 2, 1);
    m_pLocation1 = collection.addSourceLocation(LOCATION_TOKEN, 2, {1}, filePath, 4, 1, 1, 10);
  }

  SourceLocationCollection collection;
  SourceLocation* m_pLocation0 = nullptr;
  SourceLocation* m_pLocation1 = nullptr;
};

// NOLINTNEXTLINE
TEST_F(SourceLocationsDoNotGetCreatedWithWrongInput, goodCase) {
  ASSERT_EQ(nullptr, m_pLocation0);
  ASSERT_EQ(nullptr, m_pLocation1);
}

class SourceLocationsGetUniqueIdButBothEndsHaveTheSame : public testing::Test {
public:
  void SetUp() override {
    FilePath filePath(L"file.c");
    m_pLocation0 = collection.addSourceLocation(LOCATION_TOKEN, 1, {1}, filePath, 1, 1, 1, 1);
    ASSERT_NE(nullptr, m_pLocation0);
    m_pLocation1 = collection.addSourceLocation(LOCATION_TOKEN, 2, {2}, filePath, 1, 1, 1, 1);
    ASSERT_NE(nullptr, m_pLocation1);
    m_pLocation2 = collection.addSourceLocation(LOCATION_TOKEN, 3, {3}, filePath, 1, 1, 1, 1);
    ASSERT_NE(nullptr, m_pLocation2);
  }

  SourceLocationCollection collection;
  SourceLocation* m_pLocation0 = nullptr;
  SourceLocation* m_pLocation1 = nullptr;
  SourceLocation* m_pLocation2 = nullptr;
};

// NOLINTNEXTLINE
TEST_F(SourceLocationsGetUniqueIdButBothEndsHaveTheSame, goodCase) {
  EXPECT_EQ(1, collection.getSourceLocationFileCount());
  EXPECT_EQ(3, collection.getSourceLocationCount());

  EXPECT_EQ(1, m_pLocation0->getLocationId());
  EXPECT_EQ(2, m_pLocation1->getLocationId());
  EXPECT_EQ(3, m_pLocation2->getLocationId());

  EXPECT_NE(m_pLocation0->getLocationId(), m_pLocation1->getLocationId());
  EXPECT_NE(m_pLocation1->getLocationId(), m_pLocation2->getLocationId());
  EXPECT_NE(m_pLocation2->getLocationId(), m_pLocation0->getLocationId());

  EXPECT_EQ(m_pLocation0->getLocationId(), m_pLocation0->getOtherLocation()->getLocationId());
  EXPECT_EQ(m_pLocation1->getLocationId(), m_pLocation1->getOtherLocation()->getLocationId());
  EXPECT_EQ(m_pLocation2->getLocationId(), m_pLocation2->getOtherLocation()->getLocationId());
}

class SourceLocationsHaveRightFilePathLineColumnAndTokenId : public testing::Test {
public:
  void SetUp() override {
    m_pLocation = collection.addSourceLocation(LOCATION_TOKEN, 1, {1}, FilePath(L"file.c"), 2, 3, 4, 5);
    ASSERT_NE(nullptr, m_pLocation);
  }

  SourceLocationCollection collection;
  SourceLocation* m_pLocation = nullptr;
};

// NOLINTNEXTLINE
TEST_F(SourceLocationsHaveRightFilePathLineColumnAndTokenId, goodCase) {
  EXPECT_EQ(1, m_pLocation->getTokenIds()[0]);
  EXPECT_EQ(2, m_pLocation->getLineNumber());
  EXPECT_EQ(3, m_pLocation->getColumnNumber());
  EXPECT_EQ(4, m_pLocation->getOtherLocation()->getLineNumber());
  EXPECT_EQ(5, m_pLocation->getOtherLocation()->getColumnNumber());
  EXPECT_THAT(m_pLocation->getFilePath().wstr(), testing::StrEq(L"file.c"));
}

class FindingSourceLocationsById : public testing::Test {
public:
  void SetUp() override {
    FilePath filePath(L"file.c");
    m_pLocation0 = collection.addSourceLocation(LOCATION_TOKEN, 1, {1}, filePath, 2, 3, 4, 5);
    ASSERT_NE(nullptr, m_pLocation0);
    m_pLocation1 = collection.addSourceLocation(LOCATION_TOKEN, 2, {6}, filePath, 7, 8, 9, 10);
    ASSERT_NE(nullptr, m_pLocation1);
  }

  SourceLocationCollection collection;
  SourceLocation* m_pLocation0 = nullptr;
  SourceLocation* m_pLocation1 = nullptr;
};

// NOLINTNEXTLINE
TEST_F(FindingSourceLocationsById, goodCase) {
  EXPECT_EQ(m_pLocation0, collection.getSourceLocationById(m_pLocation0->getLocationId()));
  EXPECT_EQ(m_pLocation1, collection.getSourceLocationById(m_pLocation1->getLocationId()));
}

class CreatingPlainCopyOfAllLocationsInLineRange : public testing::Test {
public:
  void SetUp() override {
    const FilePath filePath{L"file.c"};
    m_pLocation0 = collection.addSourceLocation(LOCATION_TOKEN, 1, {1}, filePath, 2, 3, 4, 5);
    ASSERT_NE(nullptr, m_pLocation0);
    m_pLocation1 = collection.addSourceLocation(LOCATION_TOKEN, 2, {1}, filePath, 3, 3, 4, 5);
    ASSERT_NE(nullptr, m_pLocation1);
    m_pLocation2 = collection.addSourceLocation(LOCATION_TOKEN, 3, {1}, filePath, 1, 3, 5, 5);
    ASSERT_NE(nullptr, m_pLocation2);
    m_pLocation3 = collection.addSourceLocation(LOCATION_TOKEN, 4, {1}, filePath, 1, 5, 4, 5);
    ASSERT_NE(nullptr, m_pLocation3);
  }

  SourceLocationCollection collection;
  SourceLocation* m_pLocation0 = nullptr;
  SourceLocation* m_pLocation1 = nullptr;
  SourceLocation* m_pLocation2 = nullptr;
  SourceLocation* m_pLocation3 = nullptr;
};

// NOLINTNEXTLINE
TEST_F(CreatingPlainCopyOfAllLocationsInLineRange, goodCase) {
  Id ida = m_pLocation0->getLocationId();
  Id idb = m_pLocation1->getLocationId();
  Id idc = m_pLocation2->getLocationId();
  Id idd = m_pLocation3->getLocationId();

  unsigned int fromLine = 2;
  unsigned int toLine = 4;

  SourceLocationCollection copy;
  auto* pLocation = collection.getSourceLocationById(ida);
  ASSERT_NE(pLocation, nullptr);

  pLocation->getSourceLocationFile()->forEachSourceLocation([&copy, fromLine, toLine](SourceLocation* location) {
    if(location->getLineNumber() >= fromLine && location->getLineNumber() <= toLine) {
      copy.addSourceLocationCopy(location);
    }
  });

  EXPECT_EQ(1, copy.getSourceLocationFileCount());
  EXPECT_EQ(3, copy.getSourceLocationCount());

  EXPECT_TRUE(copy.getSourceLocationById(ida));
  EXPECT_TRUE(copy.getSourceLocationById(idb));
  EXPECT_FALSE(copy.getSourceLocationById(idc));
  EXPECT_TRUE(copy.getSourceLocationById(idd));

  EXPECT_NE(m_pLocation0, copy.getSourceLocationById(ida));
  EXPECT_NE(m_pLocation3, copy.getSourceLocationById(idd));

  EXPECT_TRUE(copy.getSourceLocationById(ida)->getStartLocation());
  EXPECT_TRUE(copy.getSourceLocationById(ida)->getEndLocation());

  EXPECT_FALSE(copy.getSourceLocationById(idd)->getStartLocation());
  EXPECT_TRUE(copy.getSourceLocationById(idd)->getEndLocation());
}

class GetSourceLocationsFilteredByLines : public testing::Test {
public:
  void SetUp() override {
    const FilePath filePath{L"file.c"};
    m_pLocation0 = collection.addSourceLocation(LOCATION_TOKEN, 1, {1}, filePath, 1, 3, 1, 5);
    ASSERT_NE(nullptr, m_pLocation0);
    m_pLocation1 = collection.addSourceLocation(LOCATION_TOKEN, 2, {1}, filePath, 1, 3, 2, 5);
    ASSERT_NE(nullptr, m_pLocation1);
    m_pLocation2 = collection.addSourceLocation(LOCATION_TOKEN, 3, {1}, filePath, 2, 3, 2, 5);
    ASSERT_NE(nullptr, m_pLocation2);
    m_pLocation3 = collection.addSourceLocation(LOCATION_TOKEN, 4, {1}, filePath, 3, 3, 4, 5);
    ASSERT_NE(nullptr, m_pLocation3);
    m_pLocation4 = collection.addSourceLocation(LOCATION_TOKEN, 5, {1}, filePath, 3, 5, 5, 5);
    ASSERT_NE(nullptr, m_pLocation4);
    m_pLocation5 = collection.addSourceLocation(LOCATION_TOKEN, 6, {1}, filePath, 1, 5, 5, 5);
    ASSERT_NE(nullptr, m_pLocation5);
    m_pLocation6 = collection.addSourceLocation(LOCATION_TOKEN, 7, {1}, filePath, 5, 5, 5, 5);
    ASSERT_NE(nullptr, m_pLocation6);
  }

  SourceLocationCollection collection;
  SourceLocation* m_pLocation0 = nullptr;
  SourceLocation* m_pLocation1 = nullptr;
  SourceLocation* m_pLocation2 = nullptr;
  SourceLocation* m_pLocation3 = nullptr;
  SourceLocation* m_pLocation4 = nullptr;
  SourceLocation* m_pLocation5 = nullptr;
  SourceLocation* m_pLocation6 = nullptr;
};

// NOLINTNEXTLINE
TEST_F(GetSourceLocationsFilteredByLines, goodCase) {
  SourceLocationCollection copy;
  copy.addSourceLocationFile(
      collection.getSourceLocationById(m_pLocation0->getLocationId())->getSourceLocationFile()->getFilteredByLines(2, 4));

  EXPECT_EQ(1, copy.getSourceLocationFileCount());
  EXPECT_EQ(4, copy.getSourceLocationCount());

  EXPECT_FALSE(copy.getSourceLocationById(m_pLocation0->getLocationId()));
  EXPECT_TRUE(copy.getSourceLocationById(m_pLocation1->getLocationId()));
  EXPECT_TRUE(copy.getSourceLocationById(m_pLocation2->getLocationId()));
  EXPECT_TRUE(copy.getSourceLocationById(m_pLocation3->getLocationId()));
  EXPECT_TRUE(copy.getSourceLocationById(m_pLocation4->getLocationId()));
  EXPECT_FALSE(copy.getSourceLocationById(m_pLocation5->getLocationId()));
  EXPECT_FALSE(copy.getSourceLocationById(m_pLocation6->getLocationId()));

  EXPECT_NE(m_pLocation1, copy.getSourceLocationById(m_pLocation1->getLocationId()));
  EXPECT_NE(m_pLocation2, copy.getSourceLocationById(m_pLocation2->getLocationId()));

  EXPECT_FALSE(copy.getSourceLocationById(m_pLocation1->getLocationId())->getStartLocation());
  EXPECT_TRUE(copy.getSourceLocationById(m_pLocation1->getLocationId())->getEndLocation());

  EXPECT_TRUE(copy.getSourceLocationById(m_pLocation4->getLocationId())->getStartLocation());
  EXPECT_FALSE(copy.getSourceLocationById(m_pLocation4->getLocationId())->getEndLocation());
}