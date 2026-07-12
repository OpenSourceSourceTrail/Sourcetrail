#pragma once

#include "QtProgressBarDialog.h"

class QLabel;
class FilePath;

class QtIndexingProgressDialog : public QtProgressBarDialog {
  Q_OBJECT

public:
  explicit QtIndexingProgressDialog(bool hideable, QWidget* parent = nullptr);
  [[nodiscard]] QSize sizeHint() const override;

  void updateIndexingProgress(size_t fileCount, size_t totalFileCount, const FilePath& sourcePath);
  void updateErrorCount(size_t errorCount, size_t fatalCount);

protected:
  void closeEvent(QCloseEvent* event) override;
  void setGeometries() override;

private:
  void onHidePressed();
  void onStopPressed();

  QLabel* m_filePathLabel;
  QWidget* m_errorWidget;
  QString m_sourcePath;
};