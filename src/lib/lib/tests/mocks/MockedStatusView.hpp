#pragma once
#include <gmock/gmock.h>

#include "StatusView.h"

struct MockedStatusView : StatusView {
  MockedStatusView(ViewLayout* viewLayout) : StatusView(viewLayout) {}
  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));
  MOCK_METHOD(void, addStatus, (const std::vector<Status>&), (override));
  MOCK_METHOD(void, clear, (), (override));
};