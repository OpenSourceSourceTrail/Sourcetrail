#pragma once
#include <functional>

class ScopedFunctor final {
public:
  ScopedFunctor();
  explicit ScopedFunctor(std::function<void(void)> onDestroy);
  ~ScopedFunctor();

private:
  std::function<void(void)> mOnDestroy;
};
