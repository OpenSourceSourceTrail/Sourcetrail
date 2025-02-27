#pragma once
// Qt5
#include <QSizeGrip>
#include <QWidget>
// internal
#include "QtWindowStack.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QMouseEvent;

class QtWindowBase : public QtWindowStackElement {
  // NOLINTNEXTLINE
  Q_OBJECT

public:
  QtWindowBase(bool isSubWindow, QWidget* parent);

  [[nodiscard]] QSize sizeHint() const override;

  void setSizeGripStyle(bool isBlack);

  [[nodiscard]] bool isSubWindow() const;

  void moveToCenter();

  // QtWindowStackElement implementation
  void showWindow() override;
  void hideWindow() override;

protected:
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

  bool m_isSubWindow;

  QWidget* m_window = nullptr;
  QWidget* m_content = nullptr;

  QPoint m_dragPosition;
  bool m_mousePressedInWindow = false;

  QSizeGrip* m_sizeGrip = nullptr;
};
