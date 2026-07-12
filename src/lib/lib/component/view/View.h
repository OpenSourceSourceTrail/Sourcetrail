#pragma once
// STL
#include <cstddef>
#include <memory>
#include <string>
// internal
#include "Component.h"
#include "ViewLayout.h"

class ViewWidgetWrapper;

template <typename ControllerType>
class ControllerProxy;

class View {
public:
  template <typename T, typename... Args>
  static std::shared_ptr<T> create(ViewLayout* viewLayout, const Args... args);

  template <typename T, typename... Args>
  static std::shared_ptr<T> createAndAddToLayout(ViewLayout* viewLayout, const Args... args);

  explicit View(ViewLayout* pViewLayout);
  virtual ~View();

  [[nodiscard]] virtual std::string getName() const = 0;

  virtual void createWidgetWrapper() = 0;
  virtual void refreshView() = 0;

  void addToLayout();
  void showDockWidget();

  void setComponent(Component* component);

  [[nodiscard]] ViewWidgetWrapper* getWidgetWrapper() const;
  [[nodiscard]] ViewLayout* getViewLayout() const;

  void setEnabled(bool enabled);

protected:
  template <typename ControllerType>
  ControllerType* getController();

  void setWidgetWrapper(std::shared_ptr<ViewWidgetWrapper> widgetWrapper);

private:
  template <typename ControllerType>
  friend class ControllerProxy;

  Component* m_component = nullptr;
  ViewLayout* const m_viewLayout;
  std::shared_ptr<ViewWidgetWrapper> m_widgetWrapper;
};

template <typename T, typename... Args>
std::shared_ptr<T> View::create(ViewLayout* viewLayout, const Args... args) {
  std::shared_ptr<T> ptr = std::make_shared<T>(viewLayout, args...);

  ptr->createWidgetWrapper();

  return ptr;
}

template <typename T, typename... Args>
std::shared_ptr<T> View::createAndAddToLayout(ViewLayout* viewLayout, const Args... args) {
  std::shared_ptr<T> ptr = View::create<T, Args...>(viewLayout, args...);

  ptr->addToLayout();

  return ptr;
}

template <typename ControllerType>
ControllerType* View::getController() {
  if(m_component != nullptr) {
    return m_component->getController<ControllerType>();
  }
  return nullptr;
}