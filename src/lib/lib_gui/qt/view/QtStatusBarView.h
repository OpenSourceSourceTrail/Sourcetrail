#pragma once

#include <memory>
#include <string>

#include "ErrorCountInfo.h"
#include "QtThreadedFunctor.h"
#include "StatusBarView.h"

class QtStatusBar;

class QtStatusBarView : public StatusBarView {
public:
  explicit QtStatusBarView(ViewLayout* viewLayout);
  ~QtStatusBarView() override;

  // View implementation
  void createWidgetWrapper() override;
  void refreshView() override;

  // StatusBar view implementation
  void showMessage(const std::wstring& message, bool isError, bool showLoader) override;
  void setErrorCount(ErrorCountInfo errorCount) override;

  void showIdeStatus(const std::wstring& message) override;

  void showIndexingProgress(size_t progressPercent) override;
  void hideIndexingProgress() override;

private:
  QtThreadedLambdaFunctor m_onQtThread;

  QtStatusBar* m_widget;
};
