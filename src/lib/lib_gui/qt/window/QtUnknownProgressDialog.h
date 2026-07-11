#pragma once
#include "QtProgressBarDialog.h"

class QtUnknownProgressDialog : public QtProgressBarDialog {
  Q_OBJECT

public:
  explicit QtUnknownProgressDialog(bool hideable, QWidget* parent = nullptr);
  [[nodiscard]] QSize sizeHint() const override;

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void onHidePressed();
};