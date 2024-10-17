#include "Component.h"

#include <utility>

#include "Controller.h"
#include "logging.h"
#include "View.h"

Component::Component(std::shared_ptr<View> view, std::shared_ptr<Controller> controller)
    : m_controller(std::move(controller)), m_view(std::move(view)) {
  if(m_controller) {
    m_controller->setComponent(this);
  } else {
    LOG_WARNING("Can not set Component to the controller.");
  }

  if(m_view) {
    m_view->setComponent(this);
  } else {
    LOG_WARNING("Can not set Component to the view.");
  }
}

Component::~Component() {
  if(m_controller) {
    m_controller->setComponent(nullptr);
  }

  if(m_view) {
    m_view->setComponent(nullptr);
  }
}

Controller* Component::getControllerPtr() const {
  return m_controller.get();
}

View* Component::getViewPtr() const {
  return m_view.get();
}