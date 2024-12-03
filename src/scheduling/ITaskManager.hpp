#pragma once
#include <memory>

#include "GlobalId.hpp"

class TaskScheduler;

namespace scheduling {

struct ITaskManager {
  using Ptr = std::shared_ptr<ITaskManager>;
  using RawPtr = ITaskManager*;

  static void setInstance(Ptr instance) noexcept;

  static Ptr getInstance() noexcept;

  static RawPtr getInstanceRaw() noexcept;

  virtual ~ITaskManager() noexcept;

  virtual std::shared_ptr<TaskScheduler> createScheduler(GlobalId schedulerId) noexcept = 0;

  virtual void destroyScheduler(GlobalId schedulerId) noexcept = 0;

  virtual std::shared_ptr<TaskScheduler> getScheduler(GlobalId schedulerId) noexcept = 0;

private:
  static Ptr sInstance;
};

}    // namespace scheduling