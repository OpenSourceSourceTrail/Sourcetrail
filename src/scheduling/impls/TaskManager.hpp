#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "../ITaskManager.hpp"
#include "types.h"

namespace scheduling::impls {

class TaskManager final : public ITaskManager {
public:
  std::shared_ptr<TaskScheduler> createScheduler(GlobalId schedulerId) noexcept override;

  void destroyScheduler(GlobalId schedulerId) noexcept override;

  std::shared_ptr<TaskScheduler> getScheduler(GlobalId schedulerId) noexcept override;

private:
  std::map<GlobalId, std::shared_ptr<TaskScheduler>> mSchedulers;
  std::mutex mSchedulersMutex;
};

}    // namespace lib::detail