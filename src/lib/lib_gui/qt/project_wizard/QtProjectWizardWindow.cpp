#include "QtProjectWizardWindow.h"
// Qt5
#include <QFrame>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
// internal
#include "QtProjectWizardContent.h"
#include "utilityQt.h"

QtProjectWizardWindow::QtProjectWizardWindow(QWidget* pParent, bool showSeparator)
    : QtWindow(false, pParent), m_content(nullptr), m_showSeparator(showSeparator) {}

QSize QtProjectWizardWindow::sizeHint() const {
  if(m_preferredSize.width() <= 1) {
    return m_preferredSize;
  }
  return QSize(750, 620);
}

QtProjectWizardContent* QtProjectWizardWindow::content() const {
  return m_content;
}

void QtProjectWizardWindow::setContent(QtProjectWizardContent* pContent) {
  m_content = pContent;
}

void QtProjectWizardWindow::setPreferredSize(QSize size) {
  m_preferredSize = size;
}

void QtProjectWizardWindow::saveContent() {
  m_content->save();
}

void QtProjectWizardWindow::loadContent() {
  m_content->load();
}

void QtProjectWizardWindow::refreshContent() {
  m_content->refresh();
}

void QtProjectWizardWindow::populateWindow(QWidget* widget) {
  auto* pLayout = new QGridLayout;
  pLayout->setContentsMargins(0, 0, 0, 0);

  pLayout->setColumnStretch(QtProjectWizardWindow::FRONT_COL, 1);
  pLayout->setColumnStretch(QtProjectWizardWindow::BACK_COL, 3);

  pLayout->setColumnStretch(HELP_COL, 0);
  pLayout->setColumnStretch(LINE_COL, 0);

  int row = 0;
  m_content->populate(pLayout, row);

  auto* pSeparator = new QFrame;
  pSeparator->setFrameShape(QFrame::VLine);

  QPalette palette = pSeparator->palette();
  palette.setColor(QPalette::WindowText, m_showSeparator ? Qt::lightGray : Qt::transparent);
  pSeparator->setPalette(palette);

  pLayout->addWidget(pSeparator, 0, LINE_COL, -1, 1);

  if(isScrollAble()) {
    pLayout->setColumnMinimumWidth(BACK_COL + 1, 10);
  }

  widget->setLayout(pLayout);
}

void QtProjectWizardWindow::windowReady() {
  updateTitle(QStringLiteral("NEW SOURCE GROUP"));

  m_content->windowReady();
}

void QtProjectWizardWindow::handleNext() {
  if(m_content != nullptr) {
    saveContent();

    if(!m_content->check()) {
      return;
    }
  }

  emit next();
}

void QtProjectWizardWindow::handlePrevious() {
  if(m_content != nullptr) {
    m_content->save();
  }

  emit previous();
}
