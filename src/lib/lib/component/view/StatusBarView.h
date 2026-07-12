#pragma once

#include "ErrorCountInfo.h"
#include "View.h"

class StatusBarController;

class StatusBarView : public View {
public:
  explicit StatusBarView(ViewLayout* viewLayout);
  ~StatusBarView() override;

  [[nodiscard]] std::string getName() const override;
  virtual void showMessage(const std::wstring& message, bool isError, bool showLoader) = 0;
  virtual void setErrorCount(ErrorCountInfo errorCount) = 0;

  virtual void showIdeStatus(const std::wstring& message) = 0;

  virtual void showIndexingProgress(size_t progressPercent) = 0;
  virtual void hideIndexingProgress() = 0;

protected:
  [[nodiscard]] StatusBarController* getStatusBarController();
};
