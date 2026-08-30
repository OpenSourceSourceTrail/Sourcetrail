#pragma once
#include <gmock/gmock.h>

#include "component/view/UndoRedoView.h"

struct MockedUndoRedoView final : UndoRedoView {
  MockedUndoRedoView(ViewLayout* viewLayout) : UndoRedoView(viewLayout) {}

  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, setRedoButtonEnabled, (bool), (override));
  MOCK_METHOD(void, setUndoButtonEnabled, (bool), (override));

  MOCK_METHOD(void, updateHistory, (const std::vector<SearchMatch>&, size_t), (override));
};