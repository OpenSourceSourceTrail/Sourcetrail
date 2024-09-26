#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
  auto logger = spdlog::default_logger_raw();
  logger->set_level(spdlog::level::off);
  testing::InitGoogleMock(&argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}