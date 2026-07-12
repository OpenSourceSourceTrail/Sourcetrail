#ifndef QT_ICON_STATE_BUTTON_H
#define QT_ICON_STATE_BUTTON_H

#include <QPushButton>

#include "FilePath.h"

class QtIconStateButton : public QPushButton {
  Q_OBJECT

public:
  enum ButtonState { STATE_DEFAULT, STATE_HOVERED, STATE_DISABLED };

  struct State {
    FilePath iconPath;
    QColor color;
  };

  QtIconStateButton(QWidget* parent = nullptr);

  void addState(ButtonState buttonState, const FilePath& iconPath, QColor color = Qt::transparent);

  void hoverIn();
  void hoverOut();

signals:
  void hoveredIn(QPushButton*);
  void hoveredOut(QPushButton*);

protected:
  void changeEvent(QEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;

private:
  void setState(const State& state);

  std::map<ButtonState, State> m_states;
};

#endif    // QT_ICON_STATE_BUTTON_H
