#pragma once
#include <QObject>

class MouseReleaseFilter : public QObject {
  Q_OBJECT

public:
  explicit MouseReleaseFilter(QObject* parent);

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

private:
  size_t m_backButton;
  size_t m_forwardButton;
};
