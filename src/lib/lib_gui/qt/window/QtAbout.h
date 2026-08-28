#ifndef QT_ABOUT_H
#define QT_ABOUT_H

#include "qt/window/QtWindow.h"

class QtAbout : public QtWindow {
  Q_OBJECT

public:
  QtAbout(QWidget* parent = nullptr);
  [[nodiscard]] QSize sizeHint() const override;

  void setupAbout();
};

#endif    // QT_ABOUT_H
