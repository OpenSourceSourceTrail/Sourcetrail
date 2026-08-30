#pragma once
#include <gmock/gmock.h>

#include "component/view/BookmarkButtonsView.h"

struct MockedBookmarkButtonsView final : BookmarkButtonsView {
  explicit MockedBookmarkButtonsView(ViewLayout* viewLayout) : BookmarkButtonsView(viewLayout) {}

  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, setCreateButtonState, (const MessageBookmarkButtonState::ButtonState&), (override));
};
