#include "ScopedFunctor.h"

ScopedFunctor::ScopedFunctor() = default;

ScopedFunctor::ScopedFunctor(std::function<void(void)> onDestroy) : mOnDestroy(std::move(onDestroy)) {}

ScopedFunctor::~ScopedFunctor() {
  if(mOnDestroy) {
    mOnDestroy();
  }
}
