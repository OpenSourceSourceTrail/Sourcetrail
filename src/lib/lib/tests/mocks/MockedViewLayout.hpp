#pragma once
#include <gmock/gmock.h>

#include "ViewLayout.h"

struct MockedViewLayout : ViewLayout {
  MOCK_METHOD(void, addView, (View*), (override));

  MOCK_METHOD(void, overrideView, (View*), (override));

  MOCK_METHOD(void, removeView, (View*), (override));

  MOCK_METHOD(void, showView, (View*), (override));

  MOCK_METHOD(void, hideView, (View*), (override));

  MOCK_METHOD(void, setViewEnabled, (View*, bool), (override));
};