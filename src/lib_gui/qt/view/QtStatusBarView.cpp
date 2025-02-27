#include "QtStatusBarView.h"

#include <QStatusBar>

#include "logging.h"
#include "QtMainView.h"
#include "QtStatusBar.h"

QtStatusBarView::QtStatusBarView(ViewLayout* viewLayout) : StatusBarView(viewLayout), m_widget(new QtStatusBar) {
  m_widget->show();

  // TODO(Hussein): Find anther way to set StatusBar
  if(auto* mainView = dynamic_cast<QtMainView*>(viewLayout); mainView != nullptr) {
    mainView->setStatusBar(m_widget);
  } else {
    LOG_WARNING("Can't cast 'viewlayout' to 'QtMainView'");
  }
}

QtStatusBarView::~QtStatusBarView() = default;

void QtStatusBarView::createWidgetWrapper() {}

void QtStatusBarView::refreshView() {}

void QtStatusBarView::showMessage(const std::wstring& message, bool isError, bool showLoader) {
  m_onQtThread([this, message, isError, showLoader]() { m_widget->setText(message, isError, showLoader); });
}

void QtStatusBarView::setErrorCount(ErrorCountInfo errorCount) {
  m_onQtThread([this, errorCount]() { m_widget->setErrorCount(errorCount); });
}

void QtStatusBarView::showIdeStatus(const std::wstring& message) {
  m_onQtThread([this, message]() { m_widget->setIdeStatus(message); });
}

void QtStatusBarView::showIndexingProgress(size_t progressPercent) {
  m_onQtThread([this, progressPercent]() { m_widget->showIndexingProgress(progressPercent); });
}

void QtStatusBarView::hideIndexingProgress() {
  m_onQtThread([this]() { m_widget->hideIndexingProgress(); });
}
