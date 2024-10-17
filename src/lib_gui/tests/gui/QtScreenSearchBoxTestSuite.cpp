#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTestEvent>

#include <gtest/gtest.h>

#include "ControllerProxy.h"
#include "gui/utilities.hpp"
#include "MockedView.hpp"
#include "MockedViewLayout.hpp"
#include "QtScreenSearchBox.h"
#include "QtSelfRefreshIconButton.h"
#include "ScreenSearchController.h"

struct QtScreenSearchBoxFix : testing::Test {
  void SetUp() override {
    viewLayout = std::make_unique<MockedViewLayout>();
    view = std::make_unique<MockedView>(viewLayout.get());
    controllerProxy = std::make_unique<ControllerProxy<ScreenSearchController>>(view.get(), TabId::app());
    searchBox = std::make_unique<QtScreenSearchBox>(controllerProxy.get());
  }

  void TearDown() override {
    viewLayout.reset();
    view.reset();
    controllerProxy.reset();
    searchBox.reset();
  }

  std::unique_ptr<MockedViewLayout> viewLayout;
  std::unique_ptr<MockedView> view;
  std::unique_ptr<ControllerProxy<ScreenSearchController>> controllerProxy;
  std::unique_ptr<QtScreenSearchBox> searchBox;
};

TEST_F(QtScreenSearchBoxFix, testingStructure) {
  auto* childLayout = searchBox->findChild<QHBoxLayout*>();
  ASSERT_NE(nullptr, childLayout) << "A QHBoxLayout will be created";
  ASSERT_EQ(childLayout, searchBox->layout()) << "QHBoxLayout is the widget layout";

  auto buttons = searchBox->findChildren<QtSelfRefreshIconButton*>();
  ASSERT_EQ(buttons.size(), 5);

  std::set<std::string> buttonsNames = {"search_button", "prev_button", "next_button", "close_button", "match_label"};
  for(auto* button : buttons) {
    auto buttonName = button->objectName().toStdString();
    auto found = buttonsNames.find(buttonName);
    EXPECT_NE(found, buttonsNames.end()) << "Button name \"" << buttonName << "\" is not found in the list";
    ASSERT_NE(-1, childLayout->indexOf(button)) << "QLineEdit is member of the layout";
  }

  auto layouts = searchBox->findChildren<QHBoxLayout*>();
  ASSERT_EQ(layouts.size(), 2);

  auto* searchEdit = searchBox->findChild<QLineEdit*>();
  EXPECT_NE(nullptr, searchEdit) << "QLineEdit is created";
  EXPECT_NE(-1, childLayout->indexOf(searchEdit)) << "QLineEdit is member of the layout";

  // TODO: Check install event filter
}


TEST_F(QtScreenSearchBoxFix, testingSetMatchCountZero) {
  searchBox->setMatchCount(0);
  const std::set<std::string> buttonsNames = {"prev_button", "next_button"};
  auto buttons = searchBox->findChildren<QtSelfRefreshIconButton*>();
  for(auto* button : buttons) {
    auto buttonName = button->objectName().toStdString();
    auto found = buttonsNames.find(buttonName);
    if(found == buttonsNames.end()) {
      continue;
    }
    ASSERT_FALSE(button->isEnabled());
  }
  const auto* matchButton = utilities::getChildByObjectName<QPushButton>(*searchBox, "match_label");
  EXPECT_NE(nullptr, matchButton);
  ASSERT_EQ(matchButton->text(), "0 matches");
}

TEST_F(QtScreenSearchBoxFix, testingSetMatchCount) {
  searchBox->setMatchCount(2);
  const std::set<std::string> buttonsNames = {"prev_button", "next_button"};
  auto buttons = searchBox->findChildren<QtSelfRefreshIconButton*>();
  for(auto* button : buttons) {
    auto buttonName = button->objectName().toStdString();
    auto found = buttonsNames.find(buttonName);
    if(found == buttonsNames.end()) {
      continue;
    }
    ASSERT_TRUE(button->isEnabled());
  }
  const auto* matchButton = utilities::getChildByObjectName<QPushButton>(*searchBox, "match_label");
  EXPECT_NE(nullptr, matchButton);
  ASSERT_EQ(matchButton->text(), "2 matches");
}

TEST_F(QtScreenSearchBoxFix, testingSetMatchIndexZero) {
  searchBox->setMatchIndex(0);
  const auto* matchButton = utilities::getChildByObjectName<QPushButton>(*searchBox, "match_label");
  EXPECT_NE(nullptr, matchButton);
  ASSERT_EQ(matchButton->text(), "0 matches");
}

TEST_F(QtScreenSearchBoxFix, testingSetMatchIndex) {
  searchBox->setMatchIndex(2);
  const auto* matchButton = utilities::getChildByObjectName<QPushButton>(*searchBox, "match_label");
  EXPECT_NE(nullptr, matchButton);
  ASSERT_EQ(matchButton->text(), "2 of 0 matches");
}


TEST_F(QtScreenSearchBoxFix, testingAddResponderEmpty) {
  searchBox->addResponder({});
  auto buttons = searchBox->findChildren<QCheckBox*>();
  ASSERT_EQ(0, buttons.size());
}

TEST_F(QtScreenSearchBoxFix, testingAddResponderExists) {
  searchBox->addResponder("code");
  auto buttons = searchBox->findChildren<QCheckBox*>();
  ASSERT_EQ(1, buttons.size());
  searchBox->addResponder("code");
  buttons = searchBox->findChildren<QCheckBox*>();
  ASSERT_EQ(1, buttons.size());
}

TEST_F(QtScreenSearchBoxFix, testingCloseSignal) {
  auto* closeButton = utilities::getChildByObjectName<QtSelfRefreshIconButton>(*searchBox, "close_button");
  EXPECT_NE(nullptr, closeButton);
  QSignalSpy spy(closeButton, &QPushButton::clicked);
  QTest::mouseClick(closeButton, Qt::MouseButton::LeftButton);
  QCOMPARE(1, spy.count());
}

TEST_F(QtScreenSearchBoxFix, testingTextChangedSignal) {
  auto* searchButton = utilities::getChildByObjectName<QLineEdit>(*searchBox, "search_box");
  QVERIFY(searchButton != nullptr);
  QTest::keyClicks(searchButton, "hello world");
  // TODO: missing checks
}

TEST_F(QtScreenSearchBoxFix, testingReturnPressedSignal) {
  auto* searchButton = utilities::getChildByObjectName<QLineEdit>(*searchBox, "search_box");
  QVERIFY(searchButton != nullptr);
  QTest::keyClick(searchButton, Qt::Key_Enter);
  QTest::keyClick(searchButton, Qt::Key_Enter, Qt::ShiftModifier);
  // TODO: missing checks
}

int main(int argc, char* argv[]) {
  const QApplication application{argc, argv};

  QTimer::singleShot(0, &application, [&]() {
    testing::InitGoogleTest(&argc, argv);
    const auto code = RUN_ALL_TESTS();
    QApplication::exit(code);
  });

  return QApplication::exec();
}
