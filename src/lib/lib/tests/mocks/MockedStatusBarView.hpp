#pragma once
#include <gmock/gmock.h>

#include "StatusBarView.h"

struct MockedStatusBarView : StatusBarView {
  MockedStatusBarView(ViewLayout* viewLayout) : StatusBarView(viewLayout) {}
  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));
  MOCK_METHOD(void, showMessage, (const std::wstring&, bool, bool), (override));
  MOCK_METHOD(void, setErrorCount, (ErrorCountInfo), (override));
  MOCK_METHOD(void, showIdeStatus, (const std::wstring&), (override));
  MOCK_METHOD(void, showIndexingProgress, (size_t), (override));
  MOCK_METHOD(void, hideIndexingProgress, (), (override));
};