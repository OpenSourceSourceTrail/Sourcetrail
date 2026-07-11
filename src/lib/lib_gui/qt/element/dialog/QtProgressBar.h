#pragma once

#include <QWidget>

#include "QtDeviceScaledPixmap.h"
#include "TimeStamp.h"

class QTimer;

class QtProgressBar : public QWidget {
  Q_OBJECT

public:
  explicit QtProgressBar(QWidget* parent = nullptr);

  void showProgress(size_t percent);
  [[nodiscard]] size_t getProgress() const;

  void showUnknownProgressAnimated();

protected:
  void paintEvent(QPaintEvent* event) override;

private slots:
  void start();
  void stop();

  void animate();

private:
  size_t m_percent;

  size_t m_count;
  QTimer* m_timer;
  TimeStamp m_TimeStamp;

  QtDeviceScaledPixmap m_pixmap;
};