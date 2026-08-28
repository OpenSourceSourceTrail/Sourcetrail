#pragma once
#include <string>
#include <vector>

#include "component/view/View.h"
#include "data/ErrorCountInfo.h"
#include "data/ErrorFilter.h"
#include "data/ErrorInfo.h"

class ErrorView : public View {
public:
  explicit ErrorView(ViewLayout* pViewLayout);

  ~ErrorView() override;

  [[nodiscard]] std::string getName() const override;

  virtual void clear() = 0;

  virtual void addErrors(const std::vector<ErrorInfo>& errors, const ErrorCountInfo& errorCount, bool scrollTo) = 0;

  virtual void setErrorId(Id errorId) = 0;

  [[nodiscard]] virtual ErrorFilter getErrorFilter() const = 0;

  virtual void setErrorFilter(const ErrorFilter& filter) = 0;

  virtual void showErrorHelpMessage() = 0;
};
