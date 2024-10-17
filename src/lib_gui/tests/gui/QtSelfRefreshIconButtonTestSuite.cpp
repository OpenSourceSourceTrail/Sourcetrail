#include "QtSelfRefreshIconButtonTestSuite.hpp"

#include <QApplication>
#include <QTest>

#include <gmock/gmock.h>

#include "../../../lib/tests/mocks/MockedMessageQueue.hpp"
#include "QtSelfRefreshIconButton.h"

QtSelfRefreshIconButtonTestSuite::QtSelfRefreshIconButtonTestSuite() = default;

QtSelfRefreshIconButtonTestSuite::~QtSelfRefreshIconButtonTestSuite() = default;

void QtSelfRefreshIconButtonTestSuite::initTestCase() {
  Q_INIT_RESOURCE(resources);
  IMessageQueue::setInstance(std::make_shared<MockedMessageQueue>());
}

void QtSelfRefreshIconButtonTestSuite::constructWithEmptyString() {
  QtSelfRefreshIconButton button({}, FilePath{}, {});

  QResizeEvent resizeEvent(QSize(800, 600), QSize(400, 300));
  QCoreApplication::sendEvent(&button, &resizeEvent);
  QTest::qSleep(50);

  QVERIFY(button.text().isEmpty());
}

void QtSelfRefreshIconButtonTestSuite::constructGoodCase() {
  QtSelfRefreshIconButton button("button", FilePath{}, {});

  QResizeEvent resizeEvent(QSize(800, 600), QSize(400, 300));
  QCoreApplication::sendEvent(&button, &resizeEvent);
  QTest::qSleep(50);

  QCOMPARE("button", button.text().toStdString());
}

void QtSelfRefreshIconButtonTestSuite::setTextWithEmptyString() {
  QtSelfRefreshIconButton button({}, FilePath{}, {});
  button.setText({});
  QVERIFY(button.text().isEmpty());
}

void QtSelfRefreshIconButtonTestSuite::setTextGoodCase() {
  QtSelfRefreshIconButton button({}, FilePath{}, {});
  button.setText("Hello");
  QCOMPARE(std::string{"Hello"}, button.text().toStdString());

  button.setAutoElide(true);
  button.setText("HelloWorld11111111111111111111111111111111111111111111111111");

  QResizeEvent resizeEvent(QSize(100, 50), button.baseSize());
  QCoreApplication::sendEvent(&button, &resizeEvent);
  QTest::qSleep(50);

  qDebug() << button.text();
  QVERIFY(button.text().contains("…1111"));
}

void QtSelfRefreshIconButtonTestSuite::setIconPath() {
  QSKIP("Add icon to resources first");
  QtSelfRefreshIconButton button({}, FilePath{}, "search/button");
  // HACK
  // auto iconPath =
  // FilePath{"../app"}.concatenate(ResourcePaths::getGuiDirectoryPath().concatenate(L"graph_view/images/graph_custom.png"));
  // button.setIconPath(iconPath);
  QVERIFY(!button.icon().isNull());
}

void QtSelfRefreshIconButtonTestSuite::setIconPathWithEmptyString() {
  QtSelfRefreshIconButton button({}, FilePath{}, "search/button");
  button.setIconPath({});
  QVERIFY(button.icon().isNull());
}

void QtSelfRefreshIconButtonTestSuite::cleanupTestCase() {
  IMessageQueue::setInstance(nullptr);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleMock(&argc, argv);
  QTEST_MAIN_IMPL(QtSelfRefreshIconButtonTestSuite)
}