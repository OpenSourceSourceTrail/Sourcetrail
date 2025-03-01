#include "QtViewWidgetWrapper.h"

#include <QWidget>

#include "logging.h"
#include "View.h"

QWidget* QtViewWidgetWrapper::getWidgetOfView(const View* view) {
  if(view == nullptr) {
    LOG_ERROR("Input argment is nullptr.");
    return nullptr;
  }

  auto* pWidgetWrapper = dynamic_cast<QtViewWidgetWrapper*>(view->getWidgetWrapper());
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

QtViewWidgetWrapper::QtViewWidgetWrapper(QWidget* widget) : mWidget(widget) {}

QtViewWidgetWrapper::~QtViewWidgetWrapper() {
  if(mWidget == nullptr) {
    LOG_WARNING("Widget is nullptr.");
    return;
  }
  mWidget->hide();
  mWidget->deleteLater();
}

QWidget* QtViewWidgetWrapper::getWidget() const {
  return mWidget;
}
