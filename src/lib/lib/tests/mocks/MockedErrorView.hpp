#pragma once
#include <gmock/gmock.h>

#include "component/view/ErrorView.h"

struct MockedErrorView : ErrorView {
  explicit MockedErrorView(ViewLayout* viewLayout) : ErrorView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, clear, (), (override));
  MOCK_METHOD(void, addErrors, (const std::vector<ErrorInfo>&, const ErrorCountInfo&, bool), (override));
  MOCK_METHOD(void, setErrorId, (Id), (override));
  MOCK_METHOD(ErrorFilter, getErrorFilter, (), (const, override));
  MOCK_METHOD(void, setErrorFilter, (const ErrorFilter&), (override));
  MOCK_METHOD(void, showErrorHelpMessage, (), (override));
};
