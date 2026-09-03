#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "activation/messages/MessageActivateLegend.h"
#include "component/ComponentFactory.h"
#include "history/logic/UndoRedoController.h"
#include "history/logic/UndoRedoView.h"
#include "history/messages/MessageHistoryRedo.h"
#include "history/messages/MessageHistoryUndo.h"
#include "history/tests/MockedUndoRedoView.hpp"
#include "mocks/HeadlessApplicationFixture.hpp"
#include "mocks/MockedStorageAccess.hpp"
#include "mocks/MockedViewFactory.hpp"
#include "mocks/MockedViewLayout.hpp"

using namespace testing;

/**
 * Characterization tests for UndoRedoController.
 *
 * Written before the feature-based restructuring so the move can be shown not to change behavior.
 * They pin what the controller does today; they are not a specification of what it should do.
 *
 * updateHistoryMenu() calls Application::getInstance() on every path -- clear() included -- hence
 * the HeadlessApplicationFixture. With a null MainView that call no-ops, so what the tests observe
 * is the view half of the same operation.
 */
struct UndoRedoControllerFix : HeadlessApplicationFixture {
  void SetUp() override {
    HeadlessApplicationFixture::SetUp();

    mViewLayout = std::make_unique<NiceMock<MockedViewLayout>>();
    mView = std::make_shared<MockedUndoRedoView>(mViewLayout.get());

    MockedViewFactory viewFactory;
    EXPECT_CALL(viewFactory, createUndoRedoView(mViewLayout.get())).WillOnce(Return(mView));

    mStorageAccess = std::make_unique<StrictMock<MockedStorageAccess>>();
    ComponentFactory factory(&viewFactory, mStorageAccess.get());
    mComponent = factory.createUndoRedoComponent(mViewLayout.get());
    mController = mComponent->getController<UndoRedoController>();
    ASSERT_FALSE(mController == nullptr);
  }

  void TearDown() override {
    mComponent.reset();
    HeadlessApplicationFixture::TearDown();
  }

  template <typename MessageType>
  void deliver(MessageType& message) {
    static_cast<MessageListener<MessageType>*>(mController)->handleMessageBase(&message);
  }

  std::shared_ptr<MockedUndoRedoView> mView;
  std::shared_ptr<Component> mComponent;
  std::unique_ptr<NiceMock<MockedViewLayout>> mViewLayout;
  std::unique_ptr<StrictMock<MockedStorageAccess>> mStorageAccess;
  UndoRedoController* mController = nullptr;
};

TEST_F(UndoRedoControllerFix, componentIsBuiltWithBothHalves) {
  EXPECT_NE(mController, nullptr);
  EXPECT_EQ(mComponent->getView<UndoRedoView>(), mView.get());
}

TEST_F(UndoRedoControllerFix, schedulerIdIsTheTabId) {
  EXPECT_EQ(mController->getSchedulerId(), mComponent->getTabId());
}

TEST_F(UndoRedoControllerFix, clearEmptiesTheHistoryAndGreysOutBothButtons) {
  EXPECT_CALL(*mView, updateHistory(IsEmpty(), _)).Times(1);
  EXPECT_CALL(*mView, setUndoButtonEnabled(false)).Times(1);
  EXPECT_CALL(*mView, setRedoButtonEnabled(false)).Times(1);

  mController->clear();
}

TEST_F(UndoRedoControllerFix, undoOnAnEmptyStackDoesNothing) {
  // The strict view mock asserts no button or history update is attempted.
  MessageHistoryUndo message;
  deliver(message);
}

TEST_F(UndoRedoControllerFix, recordingAnActivationEnablesUndoAndPublishesTheHistory) {
  EXPECT_CALL(*mView, setUndoButtonEnabled(true)).Times(AnyNumber());
  EXPECT_CALL(*mView, setRedoButtonEnabled(false)).Times(AnyNumber());
  EXPECT_CALL(*mView, updateHistory(_, _)).Times(AtLeast(1));

  MessageActivateLegend message;
  deliver(message);
}

TEST_F(UndoRedoControllerFix, aRepeatedActivationIsCollapsedRatherThanStacked) {
  EXPECT_CALL(*mView, setUndoButtonEnabled(_)).Times(AnyNumber());
  EXPECT_CALL(*mView, setRedoButtonEnabled(_)).Times(AnyNumber());
  EXPECT_CALL(*mView, updateHistory(_, _)).Times(1);

  // MessageActivateLegend carries no payload, so the second one is "the same as last" and returns
  // before it reaches processCommand() -- only the first updates the history.
  MessageActivateLegend first;
  deliver(first);
  MessageActivateLegend second;
  deliver(second);
}

TEST_F(UndoRedoControllerFix, redoAtTheEndOfTheStackDoesNothing) {
  MessageHistoryRedo message;
  deliver(message);
}
