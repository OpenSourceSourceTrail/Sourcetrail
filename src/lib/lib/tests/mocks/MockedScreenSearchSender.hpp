#pragma once
#include <gmock/gmock.h>

#include "component/controller/helper/ScreenSearchInterfaces.h"

struct MockedScreenSearchSender final : ScreenSearchSender {
  MOCK_METHOD(void, foundMatches, (ScreenSearchResponder*, size_t), (override));

  MOCK_METHOD(void, addResponder, (ScreenSearchResponder*), (override));
  MOCK_METHOD(void, removeResponder, (ScreenSearchResponder*), (override));

  MOCK_METHOD(void, clearMatches, (), (override));
};