#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <spdlog/spdlog.h>

#include "Profiling.h"

int main(int argc, char* argv[]) {
  // Test binaries link code that carries zones, and with TRACY_MANUAL_LIFETIME a zone reached
  // before the profiler is started is undefined behaviour. Suites run in parallel and all land on
  // the same port; the loser simply never accepts a connection, which is fine -- nobody profiles a
  // test run, this is only here so the zones are legal.
  const profiling::Scope tracyScope{profiling::DefaultPort};

  auto logger = spdlog::default_logger_raw();
  logger->set_level(spdlog::level::off);
  testing::InitGoogleMock(&argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}