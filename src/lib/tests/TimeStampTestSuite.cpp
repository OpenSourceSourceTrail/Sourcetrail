// GTest
#include <gmock/gmock.h>
#include <gtest/gtest.h>
// internal
#include "TimeStamp.h"

using namespace ::testing;

// NOLINTNEXTLINE
TEST(TimeStamp, goodCase) {
  constexpr auto TimeAsString = "2002-01-20 23:59:59.000";

  const TimeStamp timeStamp(TimeAsString);
  ASSERT_TRUE(timeStamp.isValid());

  EXPECT_THAT(timeStamp.toString(), StrEq("2002-01-20 23:59:59"));
  EXPECT_THAT(timeStamp.getDDMMYYYYString(), StrEq("20-01-2002"));
  EXPECT_THAT(timeStamp.dayOfWeek(), StrEq("Sunday"));
  EXPECT_THAT(timeStamp.dayOfWeekShort(), StrEq("Sun"));

  constexpr auto TimeAsString0 = "2002-01-21 00:00:00.000";

  const TimeStamp timeStamp0(TimeAsString0);
  EXPECT_EQ(1000, timeStamp0.deltaMS(timeStamp));
  EXPECT_EQ(1, timeStamp0.deltaS(timeStamp));

  EXPECT_FALSE(timeStamp0.isSameDay(timeStamp));
  EXPECT_TRUE(timeStamp0.isSameDay(timeStamp0));

  EXPECT_TRUE(timeStamp != timeStamp0);
  EXPECT_TRUE(timeStamp == timeStamp);

  EXPECT_TRUE(timeStamp < timeStamp0);
  EXPECT_FALSE(timeStamp > timeStamp);

  EXPECT_TRUE(timeStamp <= timeStamp0);
  EXPECT_TRUE(timeStamp >= timeStamp);

  constexpr auto TimeAsString1 = "2002-01-22 00:00:00.000";

  const TimeStamp timeStamp1(TimeAsString1);
  EXPECT_EQ(2, timeStamp1.deltaDays(timeStamp));
  EXPECT_EQ(24, timeStamp1.deltaHours(timeStamp));
}

// NOLINTNEXTLINE
TEST(TimeStamp, emptyTime) {
  {
    const TimeStamp timeStamp;
    ASSERT_FALSE(timeStamp.isValid());
    EXPECT_EQ("not-a-date-time", timeStamp.toString());
    EXPECT_EQ("not-a-date-time", timeStamp.getDDMMYYYYString());
  }
  {
    boost::posix_time::ptime pTime;
    const TimeStamp timeStamp{pTime};
    ASSERT_FALSE(timeStamp.isValid());
    EXPECT_EQ("not-a-date-time", timeStamp.toString());
    EXPECT_EQ("not-a-date-time", timeStamp.getDDMMYYYYString());
  }
  {
    const TimeStamp timeStamp("");
    ASSERT_FALSE(timeStamp.isValid());
    EXPECT_EQ("not-a-date-time", timeStamp.toString());
    EXPECT_EQ("not-a-date-time", timeStamp.getDDMMYYYYString());
  }
}