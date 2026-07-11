#pragma once
#include <gmock/gmock.h>

#include "ScreenSearchView.h"

struct MockedScreenSearchView final : ScreenSearchView {
  explicit MockedScreenSearchView(ViewLayout* viewLayout) : ScreenSearchView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));

  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, setMatchCount, (size_t), (override));
  MOCK_METHOD(void, setMatchIndex, (size_t), (override));

  MOCK_METHOD(void, addResponder, (const std::string&), (override));
};