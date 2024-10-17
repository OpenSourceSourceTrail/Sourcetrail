#include "QtViewWidgetWrapper.h"

#include <QWidget>

#include "logging.h"
#include "View.h"

QWidget* QtViewWidgetWrapper::getWidgetOfView(const View* pView) {
  if(pView == nullptr) {
    LOG_ERROR("Input argment is nullptr.");
    return nullptr;
  }

  auto* pWidgetWrapper = dynamic_cast<QtViewWidgetWrapper*>(pView->getWidgetWrapper());
  if(pWidgetWrapper == nullptr) {
    LOG_ERROR("Trying to get the qt widget of non qt view.");
    return nullptr;
  }

  if(pWidgetWrapper->getWidget() == nullptr) {
    LOG_ERROR("The QtViewWidgetWrapper is not holding a QWidget.");
    return nullptr;
  }

  return pWidgetWrapper->getWidget();
}

QtViewWidgetWrapper::QtViewWidgetWrapper(QWidget* pWidget) : m_pWidget(pWidget) {}

QtViewWidgetWrapper::~QtViewWidgetWrapper() {
  if(m_pWidget == nullptr) {
    LOG_WARNING("Widget is nullptr.");
    return;
  }
  m_pWidget->hide();
  m_pWidget->deleteLater();
}

QWidget* QtViewWidgetWrapper::getWidget() const {
  return m_pWidget;
}
