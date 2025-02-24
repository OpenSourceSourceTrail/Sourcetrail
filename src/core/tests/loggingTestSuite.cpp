#include <string>

#include <fmt/xchar.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "logging.h"

namespace {

struct LoggingFix : testing::Test {};

TEST_F(LoggingFix, GoodCaseTrace) {
  LOG_TRACE("Hello{}!", "World");
  LOG_TRACE(L"Hello{}!", L"World");
  LOG_TRACE_W(L"Hello{}!", L"World");

  LOG_TRACE("HelloWorld!");
  LOG_TRACE(L"HelloWorld!");
  LOG_TRACE_W(L"HelloWorld!");
}

TEST_F(LoggingFix, GoodCaseDebug) {
  LOG_DEBUG("Hello{}!", "World");
  LOG_DEBUG(L"Hello{}!", L"World");
  LOG_DEBUG_W(L"Hello{}!", L"World");

  LOG_DEBUG("HelloWorld!");
  LOG_DEBUG(L"HelloWorld!");
  LOG_DEBUG_W(L"HelloWorld!");
}

TEST_F(LoggingFix, GoodCaseInfo) {
  LOG_INFO("Hello{}!", "World");
  LOG_INFO(L"Hello{}!", L"World");
  LOG_INFO_W(L"Hello{}!", L"World");

  LOG_INFO("HelloWorld!");
  LOG_INFO(L"HelloWorld!");
  LOG_INFO_W(L"HelloWorld!");
}

TEST_F(LoggingFix, GoodCaseWarn) {
  LOG_WARNING("Hello{}!", "World");
  LOG_WARNING(L"Hello{}!", L"World");
  LOG_WARNING_W(L"Hello{}!", L"World");

  LOG_WARNING("HelloWorld!");
  LOG_WARNING(L"HelloWorld!");
  LOG_WARNING_W(L"HelloWorld!");
}

TEST_F(LoggingFix, GoodCaseError) {
  LOG_ERROR("Hello{}!", "World");
  LOG_ERROR(L"Hello{}!", L"World");
  LOG_ERROR_W(L"Hello{}!", L"World");

  LOG_ERROR("HelloWorld!");
  LOG_ERROR(L"HelloWorld!");
  LOG_ERROR_W(L"HelloWorld!");
}

TEST_F(LoggingFix, GoodCaseCritial) {
  LOG_CRITICAL("Hello{}!", "World");
  LOG_CRITICAL(L"Hello{}!", L"World");
  LOG_CRITICAL_W(L"Hello{}!", L"World");

  LOG_CRITICAL("HelloWorld!");
  LOG_CRITICAL(L"HelloWorld!");
  LOG_CRITICAL_W(L"HelloWorld!");
}

}    // namespace

int main(int argc, char* argv[]) {
  auto* logger = spdlog::default_logger_raw();
  logger->set_level(spdlog::level::trace);
  testing::InitGoogleMock(&argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}