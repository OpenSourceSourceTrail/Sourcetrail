#include "QtTextEditDialog.h"

#include <utility>

#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

QtTextEditDialog::QtTextEditDialog(QString title, QString description, QWidget* parent)
    : QtWindow(false, parent), m_title(std::move(title)), m_description(std::move(description)) {}

QSize QtTextEditDialog::sizeHint() const {
  return {550, 550};
}

void QtTextEditDialog::setText(const std::wstring& text) {
  m_text->setPlainText(QString::fromStdWString(text));
}

std::wstring QtTextEditDialog::getText() {
  return m_text->toPlainText().toStdWString();
}

void QtTextEditDialog::setReadOnly(bool readOnly) {
  m_text->setReadOnly(readOnly);

  if(readOnly) {
    updateNextButton(QStringLiteral("OK"));
  } else {
    updateNextButton(QStringLiteral("Save"));
  }
}

void QtTextEditDialog::populateWindow(QWidget* widget) {
  auto* layout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  layout->setContentsMargins(0, 0, 0, 0);

  auto* description = new QLabel(m_description);    // NOLINT(cppcoreguidelines-owning-memory)
  description->setObjectName(QStringLiteral("description"));
  description->setWordWrap(true);
  layout->addWidget(description);

  m_text = new QPlainTextEdit;    // NOLINT(cppcoreguidelines-owning-memory)
  m_text->setObjectName(QStringLiteral("textField"));
  m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
  m_text->setTabStopDistance(8 * m_text->fontMetrics().boundingRect('9').width());
  layout->addWidget(m_text);

  widget->setLayout(layout);
}

void QtTextEditDialog::windowReady() {
  updateNextButton(QStringLiteral("Save"));
  updateCloseButton(QStringLiteral("Cancel"));

  setPreviousVisible(false);

  updateTitle(m_title);
}
