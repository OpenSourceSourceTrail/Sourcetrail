#include "QtContextMenuTestSuite.hpp"

#include <set>
#include <string>

#include <QAction>
#include <QContextMenuEvent>
#include <QTest>

#include "QtContextMenu.h"
#include "utilities.hpp"

void QtContextMenuTestSuite::testingStructure() {
  const auto* instance = QtContextMenu::getInstance();

  auto actions = instance->findChildren<QAction*>();
  QCOMPARE(4, actions.size());

  const auto actionNames = std::set<std::string>{"Back", "Forward", "Copy Full Path", "Open Containing Folder"};

  QVERIFY(std::none_of(std::cbegin(actions), std::cend(actions), [&actionNames](const QAction* action) {
    const auto actionText = action->text().toStdString();
    return actionNames.find(actionText) == actionNames.cend();
  }));
}

void QtContextMenuTestSuite::testingEnableUndo() {
  auto* instance = QtContextMenu::getInstance();

  auto* action = utilities::getChildByText<QAction>(*instance, "Back");
  QVERIFY(action != nullptr);
  QVERIFY(action->isEnabled());

  instance->enableUndo(false);
  QVERIFY(!action->isEnabled());
}

void QtContextMenuTestSuite::testingEnableRedo() {
  auto* instance = QtContextMenu::getInstance();

  auto* action = utilities::getChildByText<QAction>(*instance, "Forward");
  QVERIFY(action != nullptr);
  QVERIFY(action->isEnabled());

  instance->enableRedo(false);
  QVERIFY(!action->isEnabled());
}

QTEST_MAIN(QtContextMenuTestSuite)