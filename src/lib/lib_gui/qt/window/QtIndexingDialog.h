#pragma once

#include "QtWindowBase.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QBoxLayout;

class QtIndexingDialog : public QtWindowBase {
  Q_OBJECT

protected:
  static QBoxLayout* createLayout(QWidget* parent);
  static QLabel* createTitleLabel(const QString& title, QBoxLayout* layout);
  static QLabel* createMessageLabel(QBoxLayout* layout);
  static QWidget* createErrorWidget(QBoxLayout* layout);
  static QLabel* createFlagLabel(QWidget* parent);

public:
  explicit QtIndexingDialog(bool isSubWindow, QWidget* parent = nullptr);
  [[nodiscard]] QSize sizeHint() const override = 0;

signals:
  void finished();
  void canceled();
  void visibleChanged(bool visible);

protected:
  void resizeEvent(QResizeEvent* event) override;

  void setupDone();

  QVBoxLayout* m_layout;
};
