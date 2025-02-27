#pragma once
#include "QtWindow.h"

class QtLicenseWindow : public QtWindow {
  Q_OBJECT
public:
  explicit QtLicenseWindow(QWidget* parent = nullptr);
  [[nodiscard]] QSize sizeHint() const override;

protected:
  // QtWindow implementation
  void populateWindow(QWidget* widget) override;
  void windowReady() override;
};
