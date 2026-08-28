#ifndef QT_HELP_BUTTON_H
#define QT_HELP_BUTTON_H

#include "qt/element/button/QtIconButton.h"
#include "qt/utility/QtHelpButtonInfo.h"

class QtHelpButton : public QtIconButton {
  Q_OBJECT

public:
  QtHelpButton(const QtHelpButtonInfo& info, QWidget* parent = nullptr);

  void setMessageBoxParent(QWidget* messageBoxParent);

private slots:
  void handleHelpPress();

private:
  QtHelpButtonInfo m_info;
  QWidget* m_messageBoxParent = nullptr;
};

#endif    // QT_HELP_BUTTON_H
