#pragma once
#include <QWidget>

class View;

class QtViewToggle : public QWidget {
  Q_OBJECT

public:
  explicit QtViewToggle(View* view, QWidget* parent = nullptr);
  void clear();

public slots:
  void toggledByAction();
  void toggledByUI();

private:
  View* m_view;
};