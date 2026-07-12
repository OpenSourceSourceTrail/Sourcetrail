#pragma once

#include "QtProgressBarDialog.h"

class QtKnownProgressDialog : public QtProgressBarDialog {
  Q_OBJECT

public:
  explicit QtKnownProgressDialog(bool hideable, QWidget* parent = nullptr);
  [[nodiscard]] QSize sizeHint() const override;

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void onHidePressed();
};