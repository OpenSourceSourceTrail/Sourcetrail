#include "QtStartScreenTestSuite.hpp"

#include <memory>

#include <QTest>

#include <spdlog/spdlog.h>

#include "QtStartScreen.hpp"

QtStartScreenTestSuite::QtStartScreenTestSuite() = default;

QtStartScreenTestSuite::~QtStartScreenTestSuite() = default;

void QtStartScreenTestSuite::init() {
  IApplicationSettings::setInstance(mMocked);
  EXPECT_CALL(*mMocked, getWindowBaseWidth()).WillOnce(testing::Return(600));
  EXPECT_CALL(*mMocked, getWindowBaseHeight()).WillOnce(testing::Return(650));

  mScreen = std::make_unique<qt::window::QtStartScreen>();
  mScreen->show();
  QVERIFY(QTest::qWaitForWindowActive(mScreen.get()));
}

void QtStartScreenTestSuite::goodCase() {
  QVERIFY(mScreen->isSubWindow());
  QCOMPARE(QSize(600, 650), mScreen->sizeHint());
}

void QtStartScreenTestSuite::cleanup() {
  IApplicationSettings::setInstance(nullptr);
  mMocked.reset();
}

int main(int argc, char* argv[]) {
  testing::InitGoogleMock(&argc, argv);
  auto* logger = spdlog::default_logger_raw();
  logger->set_level(spdlog::level::off);
  QTEST_MAIN_IMPL(QtStartScreenTestSuite)
}