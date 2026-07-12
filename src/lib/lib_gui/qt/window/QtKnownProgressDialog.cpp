#include "QtKnownProgressDialog.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

QtKnownProgressDialog::QtKnownProgressDialog(bool hideable, QWidget* parent) : QtProgressBarDialog(0.5, parent) {
  setSizeGripStyle(false);

  updateTitle(QStringLiteral("Progress"));

  m_layout->addStretch();

  if(hideable) {
    auto* buttons = new QHBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
    buttons->addStretch();

    auto* hideButton = new QPushButton(QStringLiteral("Hide"));
    hideButton->setObjectName(QStringLiteral("windowButton"));
    hideButton->setDefault(true);
    connect(hideButton, &QPushButton::clicked, this, &QtKnownProgressDialog::onHidePressed);
    buttons->addWidget(hideButton);

    m_layout->addLayout(buttons);
  }

  setupDone();
  setGeometries();
}

QSize QtKnownProgressDialog::sizeHint() const {
  return {350, 280};
}

void QtKnownProgressDialog::closeEvent(QCloseEvent* /*event*/) {
  emit visibleChanged(false);
}

void QtKnownProgressDialog::onHidePressed() {
  emit visibleChanged(false);
}
