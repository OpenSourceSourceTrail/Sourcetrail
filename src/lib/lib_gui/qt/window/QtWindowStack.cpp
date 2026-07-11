#include "QtWindowStack.h"

#include "QtWindow.h"


QtWindowStackElement::QtWindowStackElement(QWidget* parent) : QWidget(parent) {}

QtWindowStack::QtWindowStack(QObject* parent) : QObject(parent) {}

QtWindowStackElement* QtWindowStack::getTopWindow() const {
  return m_stack.empty() ? nullptr : m_stack.back();
}

QtWindowStackElement* QtWindowStack::getBottomWindow() const {
  return m_stack.empty() ? nullptr : m_stack.front();
}

size_t QtWindowStack::getWindowCount() const {
  return m_stack.size();
}

void QtWindowStack::pushWindow(QtWindowStackElement* window) {
  if(!m_stack.empty()) {
    m_stack.back()->hideWindow();
  }

  window->showWindow();

  m_stack.push_back(window);

  emit push();
}

void QtWindowStack::popWindow() {
  if(!m_stack.empty()) {
    m_stack.back()->hideWindow();
    m_stack.back()->deleteLater();
    m_stack.pop_back();

    emit pop();
  }

  if(m_stack.empty()) {
    emit empty();
  } else {
    m_stack.back()->showWindow();
  }
}

void QtWindowStack::centerSubWindows() {
  for(QtWindowStackElement* window : m_stack) {
    auto* qtWindow = dynamic_cast<QtWindow*>(window);
    if(qtWindow != nullptr) {
      if(qtWindow->isSubWindow()) {
        qtWindow->moveToCenter();
      }
    }
  }
}

void QtWindowStack::clearWindows() {
  for(QtWindowStackElement* window : m_stack) {
    window->hideWindow();
    window->deleteLater();
  }

  m_stack.clear();

  emit empty();
}
