// GTest
#include <gmock/gmock.h>
#include <gtest/gtest.h>
// internal
#include "IndexerCommandType.h"
#include "LanguageType.h"
#include "SourceGroupType.h"

using namespace ::testing;

// These three enums are serialized as raw int32 in engine.proto (CapabilitiesResponse,
// PluginInfo), so their numeric values are wire format. They must not depend on build
// options and must not be reordered: a GUI and an engine that disagree here decode each
// other's capabilities into the wrong languages, silently.

// NOLINTNEXTLINE
TEST(IndexerWireEnums, sourceGroupTypeValuesAreStable) {
  EXPECT_EQ(0, SOURCE_GROUP_C_EMPTY);
  EXPECT_EQ(1, SOURCE_GROUP_CPP_EMPTY);
  EXPECT_EQ(2, SOURCE_GROUP_CXX_CDB);
  EXPECT_EQ(3, SOURCE_GROUP_CXX_VS);
  EXPECT_EQ(4, SOURCE_GROUP_JAVA_EMPTY);
  EXPECT_EQ(5, SOURCE_GROUP_CUSTOM_COMMAND);
  EXPECT_EQ(6, SOURCE_GROUP_UNKNOWN);
}

// NOLINTNEXTLINE
TEST(IndexerWireEnums, languageTypeValuesAreStable) {
  EXPECT_EQ(0, LANGUAGE_CPP);
  EXPECT_EQ(1, LANGUAGE_C);
  EXPECT_EQ(2, LANGUAGE_JAVA);
  EXPECT_EQ(3, LANGUAGE_CUSTOM);
  EXPECT_EQ(4, LANGUAGE_UNKNOWN);
}

// NOLINTNEXTLINE
TEST(IndexerWireEnums, indexerCommandTypeValuesAreStable) {
  EXPECT_EQ(0, INDEXER_COMMAND_UNKNOWN);
  EXPECT_EQ(1, INDEXER_COMMAND_CXX);
  EXPECT_EQ(2, INDEXER_COMMAND_JAVA);
  EXPECT_EQ(3, INDEXER_COMMAND_CUSTOM);
}

// The string forms are what manifest.xml and the project file store, so they are a
// second, independent contract. Every value must round-trip regardless of build options.
// NOLINTNEXTLINE
TEST(IndexerWireEnums, everySourceGroupTypeRoundTripsThroughItsString) {
  for(int i = SOURCE_GROUP_C_EMPTY; i < SOURCE_GROUP_UNKNOWN; ++i) {
    const auto type = static_cast<SourceGroupType>(i);
    EXPECT_EQ(type, stringToSourceGroupType(sourceGroupTypeToString(type))) << "value " << i;
  }
}

// NOLINTNEXTLINE
TEST(IndexerWireEnums, everyLanguageTypeRoundTripsThroughItsString) {
  for(int i = LANGUAGE_CPP; i < LANGUAGE_UNKNOWN; ++i) {
    const auto type = static_cast<LanguageType>(i);
    EXPECT_EQ(type, stringToLanguageType(languageTypeToString(type))) << "value " << i;
  }
}

// NOLINTNEXTLINE
TEST(IndexerWireEnums, everyIndexerCommandTypeRoundTripsThroughItsString) {
  for(int i = INDEXER_COMMAND_UNKNOWN; i <= INDEXER_COMMAND_CUSTOM; ++i) {
    const auto type = static_cast<IndexerCommandType>(i);
    EXPECT_EQ(type, stringToIndexerCommandType(indexerCommandTypeToString(type))) << "value " << i;
  }
}
