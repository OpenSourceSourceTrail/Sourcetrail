#include <memory>
#include <regex>
#include <thread>

#include <range/v3/algorithm/any_of.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include "Application.h"
#include "Version.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"


using namespace std::chrono_literals;

TEST(Application, singleton) {
#ifdef ST_DEBUG
  ASSERT_DEATH(Application::getInstance(), "");
#else
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
#endif

  Application::createInstance(Version {}, nullptr, nullptr);
  auto baseApp = Application::getInstance();
  ASSERT_THAT(Application::getInstance(), testing::NotNull());

  auto apps = {Application::getInstance(), Application::getInstance(), Application::getInstance()};
  EXPECT_TRUE(ranges::cpp20::any_of(apps, [ptr = baseApp.get()](const auto& a) { return ptr == a.get(); }));

  // HACK: destory before start the event-loop will start inf-loop
  std::this_thread::sleep_for(100ms);

  Application::destroyInstance();
#ifdef ST_DEBUG
  ASSERT_DEATH(Application::getInstance(), "");
#else
  ASSERT_THAT(Application::getInstance(), testing::IsNull());
#endif
}

struct ApplicationFix : testing::Test {
  void SetUp() {
    Application::createInstance(Version {}, nullptr, nullptr);
    mApp = Application::getInstance();
  }
  void TearDown() {
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

int main(int argc, char* argv[]) {
  auto logger = spdlog::default_logger_raw();
  logger->set_level(spdlog::level::off);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
