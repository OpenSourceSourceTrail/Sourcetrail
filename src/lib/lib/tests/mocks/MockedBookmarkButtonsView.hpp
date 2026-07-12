#pragma once
#include <gmock/gmock.h>

#include "BookmarkButtonsView.h"

struct MockedBookmarkButtonsView final : BookmarkButtonsView {
  explicit MockedBookmarkButtonsView(ViewLayout* viewLayout) : BookmarkButtonsView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, setCreateButtonState, (const MessageBookmarkButtonState::ButtonState&), (override));
};
