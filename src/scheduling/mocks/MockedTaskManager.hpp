#pragma once
#include <gmock/gmock.h>

#include "ITaskManager.hpp"
#include "TaskScheduler.h"

namespace scheduling::mocks {

struct MockedTaskManager : ITaskManager {
  ~MockedTaskManager() noexcept override = default;

  MOCK_METHOD(std::shared_ptr<TaskScheduler>, createScheduler, (GlobalId), (noexcept, override));

  MOCK_METHOD(void, destroyScheduler, (GlobalId), (noexcept, override));

  MOCK_METHOD(std::shared_ptr<TaskScheduler>, getScheduler, (GlobalId), (noexcept, override));
};

}    // namespace scheduling::mocks