#include "component/view/View.h"

View::View(ViewLayout* pViewLayout) : m_viewLayout(pViewLayout) {}

View::~View() = default;

void View::addToLayout() {
  m_viewLayout->addView(this);
}

void View::showView() {
  m_viewLayout->showView(this);
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
