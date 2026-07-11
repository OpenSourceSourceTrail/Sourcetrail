#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Component.h"
#include "Controller.h"
#include "View.h"

using namespace ::testing;

struct MockedView final : public View {
  MockedView() : View(nullptr) {}
  MOCK_METHOD(std::string, getName, (), (const, override));
  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));
};
struct MockedController final : public Controller {
  MOCK_METHOD(void, clear, (), (override));
};

// NOLINTNEXTLINE
TEST(Component, goodCase) {
  auto view = std::make_shared<MockedView>();
  auto controller = std::make_shared<MockedController>();
  Component component(view, controller);
  EXPECT_EQ(view.get(), component.getViewPtr());
  EXPECT_EQ(view.get(), component.getView<MockedView>());
  EXPECT_EQ(controller.get(), component.getControllerPtr());
  EXPECT_EQ(controller.get(), component.getController<MockedController>());
}

// NOLINTNEXTLINE
TEST(Component, TabId) {
  Component component(nullptr, nullptr);
  EXPECT_EQ(nullptr, component.getView<MockedView>());
  EXPECT_EQ(nullptr, component.getController<MockedController>());
  // Default value
  EXPECT_EQ(0, component.getTabId());
  // Default value
  component.setTabId(1);
  EXPECT_EQ(1, component.getTabId());
}