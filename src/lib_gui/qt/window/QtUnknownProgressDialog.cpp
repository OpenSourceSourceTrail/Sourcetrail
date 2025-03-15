#include "QtUnknownProgressDialog.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

QtUnknownProgressDialog::QtUnknownProgressDialog(bool hideable, QWidget* parent) : QtProgressBarDialog(0.5, parent) {
  setSizeGripStyle(false);

  updateTitle(QStringLiteral("Status"));

  m_layout->addStretch();

  if(hideable) {
    auto* buttons = new QHBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory): Qt handle the memory
    buttons->addStretch();

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handle the memory
    auto* hideButton = new QPushButton{QStringLiteral("Hide")};
    hideButton->setObjectName(QStringLiteral("windowButton"));
    hideButton->setDefault(true);
    std::ignore = connect(hideButton, &QPushButton::clicked, this, &QtUnknownProgressDialog::onHidePressed);
    buttons->addWidget(hideButton);

    m_layout->addLayout(buttons);
  }

  setUnknownProgress();

  setupDone();
  setGeometries();
}

QSize QtUnknownProgressDialog::sizeHint() const {
  return {350, 280};
}

void QtUnknownProgressDialog::closeEvent(QCloseEvent* /*event*/) {
  emit visibleChanged(false);
}

void QtUnknownProgressDialog::onHidePressed() {
  emit visibleChanged(false);
}
