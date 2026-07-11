// GTest
#include <gmock/gmock.h>
#include <gtest/gtest.h>
// internal
#include "LocationType.h"

using namespace ::testing;
using namespace std::string_literals;

// NOLINTNEXTLINE
TEST(LocationType, locationTypeToInt) {
  EXPECT_EQ(0, locationTypeToInt(LocationType::LOCATION_TOKEN));
  EXPECT_EQ(1, locationTypeToInt(LocationType::LOCATION_SCOPE));
  EXPECT_EQ(2, locationTypeToInt(LocationType::LOCATION_QUALIFIER));
  EXPECT_EQ(3, locationTypeToInt(LocationType::LOCATION_LOCAL_SYMBOL));
  EXPECT_EQ(4, locationTypeToInt(LocationType::LOCATION_SIGNATURE));
  EXPECT_EQ(5, locationTypeToInt(LocationType::LOCATION_COMMENT));
  EXPECT_EQ(6, locationTypeToInt(LocationType::LOCATION_ERROR));
  EXPECT_EQ(7, locationTypeToInt(LocationType::LOCATION_FULLTEXT_SEARCH));
  EXPECT_EQ(8, locationTypeToInt(LocationType::LOCATION_SCREEN_SEARCH));
  EXPECT_EQ(9, locationTypeToInt(LocationType::LOCATION_UNSOLVED));
}

// NOLINTNEXTLINE
TEST(LocationType, intToLocationType) {
  EXPECT_EQ(LocationType::LOCATION_TOKEN, intToLocationType(-1));
  EXPECT_EQ(LocationType::LOCATION_TOKEN, intToLocationType(0));
  EXPECT_EQ(LocationType::LOCATION_SCOPE, intToLocationType(1));
  EXPECT_EQ(LocationType::LOCATION_QUALIFIER, intToLocationType(2));
  EXPECT_EQ(LocationType::LOCATION_LOCAL_SYMBOL, intToLocationType(3));
  EXPECT_EQ(LocationType::LOCATION_SIGNATURE, intToLocationType(4));
  EXPECT_EQ(LocationType::LOCATION_COMMENT, intToLocationType(5));
  EXPECT_EQ(LocationType::LOCATION_ERROR, intToLocationType(6));
  EXPECT_EQ(LocationType::LOCATION_FULLTEXT_SEARCH, intToLocationType(7));
  EXPECT_EQ(LocationType::LOCATION_SCREEN_SEARCH, intToLocationType(8));
  EXPECT_EQ(LocationType::LOCATION_UNSOLVED, intToLocationType(9));
}