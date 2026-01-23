#pragma once
#include "ViewWidgetWrapper.h"

class QWidget;
class View;

class QtViewWidgetWrapper : public ViewWidgetWrapper {
public:
  static QWidget* getWidgetOfView(const View* pView);

  explicit QtViewWidgetWrapper(QWidget* pWidget);

  ~QtViewWidgetWrapper() override;

  [[nodiscard]] QWidget* getWidget() const;

private:
  QWidget* mWidget = nullptr;
};
