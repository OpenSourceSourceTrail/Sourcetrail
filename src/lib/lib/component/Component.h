#pragma once

#include <memory>

#include "GlobalId.hpp"

class View;
class Controller;

class Component {
public:
  Component(std::shared_ptr<View> view, std::shared_ptr<Controller> controller);

  Component(const Component&) = delete;
  Component(Component&&) = delete;
  Component& operator=(const Component&) = delete;
  Component& operator=(Component&&) = delete;

  ~Component();

  template <typename ControllerType>
  ControllerType* getController() const;

  [[nodiscard]] Controller* getControllerPtr() const;

  template <typename ViewType>
  ViewType* getView() const;

  [[nodiscard]] View* getViewPtr() const;

  void setTabId(Id tabId) {
    m_tabId = tabId;
  }

  [[nodiscard]] Id getTabId() const {
    return m_tabId;
  }

private:
  std::shared_ptr<Controller> m_controller;
  std::shared_ptr<View> m_view;

  Id m_tabId = 0;
};

template <typename ControllerType>
ControllerType* Component::getController() const {
  if(m_controller) {
    return dynamic_cast<ControllerType*>(m_controller.get());
  }
  return nullptr;
}

template <typename ViewType>
ViewType* Component::getView() const {
  if(m_view) {
    return dynamic_cast<ViewType*>(m_view.get());
  }
  return nullptr;
}