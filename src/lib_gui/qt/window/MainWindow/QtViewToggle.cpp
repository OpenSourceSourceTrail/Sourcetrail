#include "QtViewToggle.hpp"

#include "QtMainWindow.h"
#include "View.h"

QtViewToggle::QtViewToggle(View* view, QWidget* parent) : QWidget(parent), m_view(view) {}

void QtViewToggle::clear() {
  m_view = nullptr;
}

void QtViewToggle::toggledByAction() {
  if(m_view != nullptr) {
    if(auto* window = dynamic_cast<QtMainWindow*>(parent()); window != nullptr) {
      window->toggleView(m_view, true);
    }
  }
}

void QtViewToggle::toggledByUI() {
  if(m_view != nullptr) {
    if(auto* window = dynamic_cast<QtMainWindow*>(parent()); window != nullptr) {
      window->toggleView(m_view, false);
    }
  }
}
