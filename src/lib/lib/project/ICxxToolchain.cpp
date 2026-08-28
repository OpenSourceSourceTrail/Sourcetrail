#include "project/ICxxToolchain.h"

std::shared_ptr<ICxxToolchain> ICxxToolchain::sInstance;

ICxxToolchain::~ICxxToolchain() = default;

ICxxToolchain* ICxxToolchain::getInstance() {
  return sInstance.get();
}

void ICxxToolchain::setInstance(std::shared_ptr<ICxxToolchain> toolchain) {
  sInstance = std::move(toolchain);
}
