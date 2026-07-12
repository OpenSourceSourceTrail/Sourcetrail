#include "View.h"
// STL
#include <utility>
// internal
#include "ViewWidgetWrapper.h"

View::View(ViewLayout* pViewLayout) : m_viewLayout(pViewLayout) {}

View::~View() = default;

void View::addToLayout() {
  m_viewLayout->addView(this);
}

void View::showDockWidget() {
  m_viewLayout->showView(this);
}

ViewWidgetWrapper* View::getWidgetWrapper() const {
  return m_widgetWrapper.get();
}

void View::setComponent(Component* component) {
  m_component = component;
}

ViewLayout* View::getViewLayout() const {
  return m_viewLayout;
}

void View::setEnabled(bool enabled) {
  return getViewLayout()->setViewEnabled(this, enabled);
}

void View::setWidgetWrapper(std::shared_ptr<ViewWidgetWrapper> widgetWrapper) {
  m_widgetWrapper = std::move(widgetWrapper);
}
