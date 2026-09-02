#include <gtest/gtest.h>

#include "project/logic/HeaderSearchDetection.h"

namespace {

// The quantile count used to be spelled inline in two wizard pages as
// `static_cast<size_t>(log2(sourceFilePaths.size()))`. On an empty source set that is `log2(0.0)`,
// which is -inf, and converting -inf to an unsigned type is undefined behavior -- reachable by
// pointing include detection at a directory holding no source files.
TEST(HeaderSearchDetectionTest, quantileCountIsZeroForNoSourceFiles) {
  EXPECT_EQ(utility::detectionQuantileCount(0), 0U);
}

TEST(HeaderSearchDetectionTest, quantileCountIsZeroForASingleSourceFile) {
  EXPECT_EQ(utility::detectionQuantileCount(1), 0U);
}

TEST(HeaderSearchDetectionTest, quantileCountIsTheLog2OfTheFileCount) {
  EXPECT_EQ(utility::detectionQuantileCount(2), 1U);
  EXPECT_EQ(utility::detectionQuantileCount(1024), 10U);
  EXPECT_EQ(utility::detectionQuantileCount(1000), 9U);
}

TEST(HeaderSearchDetectionTest, collectingInputsFromNoSettingsFindsNothingToDetect) {
  EXPECT_FALSE(utility::collectDetectionInputs(nullptr).has_value());
}

}    // namespace
