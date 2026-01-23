#pragma once
#include <concepts>

#include "Component.h"

template <typename T>
concept IsView = std::is_base_of_v<View, T>;

class Controller {
public:
  Controller();
  virtual ~Controller();

  void setComponent(Component* component);

  virtual void clear() = 0;

  [[nodiscard]] Id getTabId() const;

protected:
  template <typename ViewType>
  ViewType* getView() const
    requires IsView<ViewType>;

private:
  Component* mComponent = nullptr;
};

template <typename ViewType>
ViewType* Controller::getView() const
  requires IsView<ViewType>
{
  return nullptr != mComponent ? mComponent->getView<ViewType>() : nullptr;
}
