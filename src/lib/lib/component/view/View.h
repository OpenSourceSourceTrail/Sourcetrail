#pragma once
// STL
#include <memory>
#include <string>
// internal
#include "component/Component.h"
#include "component/view/ViewLayout.h"

template <typename ControllerType>
class ControllerProxy;

/**
 * The controller-facing half of a component.
 *
 * A view is the thing a controller pushes results at; how those results are painted is entirely up
 * to the implementation. Nothing here is toolkit-shaped any more -- the widget GUI's dock-widget
 * and QWidget-wrapper machinery went away with it, and the QML front end binds its view-models
 * straight to the scene instead.
 */
class View {
public:
  template <typename T, typename... Args>
  static std::shared_ptr<T> create(ViewLayout* viewLayout, const Args... args);

  template <typename T, typename... Args>
  static std::shared_ptr<T> createAndAddToLayout(ViewLayout* viewLayout, const Args... args);

  explicit View(ViewLayout* pViewLayout);
  virtual ~View();

  [[nodiscard]] virtual std::string getName() const = 0;

  virtual void refreshView() = 0;

  void addToLayout();

  /** Asks the layout to bring this view to the front. */
  void showView();

  void setComponent(Component* component);

  [[nodiscard]] ViewLayout* getViewLayout() const;

  void setEnabled(bool enabled);

protected:
  template <typename ControllerType>
  ControllerType* getController();

private:
  template <typename ControllerType>
  friend class ControllerProxy;

  Component* m_component = nullptr;
  ViewLayout* const m_viewLayout;
};

template <typename T, typename... Args>
std::shared_ptr<T> View::create(ViewLayout* viewLayout, const Args... args) {
  return std::make_shared<T>(viewLayout, args...);
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
