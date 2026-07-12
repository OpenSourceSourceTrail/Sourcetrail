#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Status.h"

using namespace ::testing;

// NOLINTNEXTLINE
TEST(Status, goodCase) {
  constexpr auto Info = L"Info";
  const Status infoStatus(Info);
  EXPECT_THAT(infoStatus.message, StrEq(Info));
  EXPECT_EQ(StatusType::Info, infoStatus.type);

  constexpr auto Error = L"Error";
  const Status errorStatus(Error, true);
  EXPECT_THAT(errorStatus.message, StrEq(Error));
  EXPECT_EQ(StatusType::Error, errorStatus.type);
}