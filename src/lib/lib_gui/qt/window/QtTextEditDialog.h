#pragma once

#include "QtWindow.h"

class QPlainTextEdit;

class QtTextEditDialog : public QtWindow {
  Q_OBJECT

public:
  QtTextEditDialog(QString title, QString description, QWidget* parent = nullptr);

  [[nodiscard]] QSize sizeHint() const override;

  void setText(const std::wstring& text);
  std::wstring getText();

  void setReadOnly(bool readOnly);

protected:
  void populateWindow(QWidget* widget) override;
  void windowReady() override;

  QString m_title;
  QString m_description;

private:
  QPlainTextEdit* m_text;
};