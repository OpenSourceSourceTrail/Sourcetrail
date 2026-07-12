#pragma once
#include "QtIndexingDialog.h"

class QLabel;
class QtProgressBar;

class QtProgressBarDialog : public QtIndexingDialog {
  Q_OBJECT

public:
  explicit QtProgressBarDialog(float topRatio, QWidget* parent = nullptr);
  void updateTitle(const QString& title);
  [[nodiscard]] std::wstring getTitle() const;
  void updateMessage(const QString& message);
  [[nodiscard]] std::wstring getMessage() const;
  void setUnknownProgress();
  void updateProgress(size_t progress);
  [[nodiscard]] size_t getProgress() const;

protected:
  void resizeEvent(QResizeEvent* event) override;
  virtual void setGeometries();

private:
  QLabel* m_title;
  QWidget* m_top;
  float m_topRatio;
  QtProgressBar* m_progressBar;
  QLabel* m_percentLabel;
  QLabel* m_messageLabel;
};
