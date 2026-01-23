#include "Controller.h"

#include "GlobalId.hpp"


Controller::Controller() = default;

Controller::~Controller() = default;

void Controller::setComponent(Component* component) {
  mComponent = component;
}

Id Controller::getTabId() const {
  return nullptr != mComponent ? mComponent->getTabId() : 0;
}
