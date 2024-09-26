#include <memory>
#include <regex>
#include <thread>

#include <range/v3/algorithm/any_of.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Application.h"
#include "MockedApplicationSetting.hpp"
#include "Version.h"


using namespace std::chrono_literals;

struct SingletonApplicationFix : testing::Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mMockedAppSettings);
  }
  void TearDown() override {
    IApplicationSettings::setInstance(nullptr);
  }

  void MockAppSettingsForGetInstance() {
    EXPECT_CALL(*mMockedAppSettings, load(testing::_, testing::_)).WillOnce(testing::Return(true));
    EXPECT_CALL(*mMockedAppSettings, getLoggingEnabled()).WillOnce(testing::Return(false));
    EXPECT_CALL(*mMockedAppSettings, getColorSchemePath()).WillOnce(testing::Return(std::filesystem::path {}));
  }

  std::shared_ptr<testing::StrictMock<MockedApplicationSettings>> mMockedAppSettings =
      std::make_shared<testing::StrictMock<MockedApplicationSettings>>();
};

TEST_F(SingletonApplicationFix, singleton) {
#if 0    // Enabled when assert function is enabled in the getInstance function
#  ifdef ST_DEBUG
  ASSERT_DEATH(Application::getInstance(), "");
#  else
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
#  endif
#endif
  ASSERT_THAT(Application::getInstance(), testing::IsNull());

  MockAppSettingsForGetInstance();

  Application::createInstance(Version {}, nullptr, nullptr);
  auto baseApp = Application::getInstance();
  ASSERT_THAT(Application::getInstance(), testing::NotNull());

  auto apps = {Application::getInstance(), Application::getInstance(), Application::getInstance()};
  EXPECT_TRUE(ranges::cpp20::any_of(apps, [ptr = baseApp.get()](const auto& a) { return ptr == a.get(); }));

  // HACK: destory before start the event-loop will start inf-loop
  std::this_thread::sleep_for(100ms);

  Application::destroyInstance();
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
#if 0    // Enabled when assert function is enabled in the getInstance function
#  ifdef ST_DEBUG
  ASSERT_DEATH(Application::getInstance(), "");
#  else
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
#  endif
#endif
}

struct ApplicationFix : SingletonApplicationFix {
  void SetUp() override {
    SingletonApplicationFix::SetUp();

    MockAppSettingsForGetInstance();

    Application::createInstance(Version {}, nullptr, nullptr);
    mApp = Application::getInstance();
  }
  void TearDown() override {
    SingletonApplicationFix::TearDown();
    // HACK: destory before start the event-loop will start inf-loop
    std::this_thread::sleep_for(100ms);
    Application::destroyInstance();
  }
  std::shared_ptr<Application> mApp;
};

TEST_F(ApplicationFix, getUUID) {
  auto uuid = mApp->getUUID();
  const std::regex mask {"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"};
  std::smatch base_match;
  std::regex_match(uuid, base_match, mask);
  EXPECT_EQ(1, base_match.size());
}