#include "QtIndexingDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>

#include "QtDeviceScaledPixmap.h"
#include "QtHelpButton.h"
#include "ResourcePaths.h"
#include "utilityQt.h"

QLabel* QtIndexingDialog::createTitleLabel(const QString& title, QBoxLayout* layout) {
  auto* label = new QLabel{title};    // NOLINT(cppcoreguidelines-owning-memory)
  label->setObjectName(QStringLiteral("title"));
  label->setAlignment(Qt::AlignRight | Qt::AlignBottom);

  if(layout != nullptr) {
    layout->addWidget(label, 0, Qt::AlignRight);
  }

  return label;
}

QLabel* QtIndexingDialog::createMessageLabel(QBoxLayout* layout) {
  auto* label = new QLabel;    // NOLINT(cppcoreguidelines-owning-memory)
  label->setObjectName(QStringLiteral("message"));
  label->setAlignment(Qt::AlignRight);
  label->setWordWrap(true);
  layout->addWidget(label);
  return label;
}

QWidget* QtIndexingDialog::createErrorWidget(QBoxLayout* layout) {
  auto* errorWidget = new QWidget;                     // NOLINT(cppcoreguidelines-owning-memory)
  auto* errorLayout = new QHBoxLayout(errorWidget);    // NOLINT(cppcoreguidelines-owning-memory)
  errorLayout->setContentsMargins(0, 0, 0, 0);
  errorLayout->setSpacing(5);

  auto* errorCount = new QPushButton();    // NOLINT(cppcoreguidelines-owning-memory)
  errorCount->setObjectName(QStringLiteral("errorCount"));
  errorCount->setAttribute(Qt::WA_LayoutUsesWidgetRect);    // fixes layouting on Mac

  errorCount->setIcon(
      QPixmap(QString::fromStdWString(ResourcePaths::getGuiDirectoryPath().concatenate(L"indexing_dialog/error.png").wstr())));
  errorLayout->addWidget(errorCount);

  auto* helpButton = new QtHelpButton(createErrorHelpButtonInfo());    // NOLINT(cppcoreguidelines-owning-memory)
  helpButton->setColor(Qt::white);
  errorLayout->addWidget(helpButton);

  layout->addWidget(errorWidget, 0, Qt::AlignRight);
  errorWidget->hide();
  return errorWidget;
}

QLabel* QtIndexingDialog::createFlagLabel(QWidget* parent) {
  QtDeviceScaledPixmap flag(
      QString::fromStdWString(ResourcePaths::getGuiDirectoryPath().concatenate(L"indexing_dialog/flag.png").wstr()));
  flag.scaleToWidth(120);

  auto* flagLabel = new QLabel{parent};    // NOLINT(cppcoreguidelines-owning-memory)
  flagLabel->setPixmap(flag.pixmap());
  flagLabel->resize(static_cast<int>(flag.width()), static_cast<int>(flag.height()));
  flagLabel->move(15, 75);
  flagLabel->show();

  return flagLabel;
}


QtIndexingDialog::QtIndexingDialog(bool isSubWindow, QWidget* parent)
    : QtWindowBase(isSubWindow, parent), m_layout(new QVBoxLayout{this}) {
  m_window->setStyleSheet(m_window->styleSheet() +
                          QStringLiteral("#window { "
                                         "background: #2E3C86;"
                                         "border: none;"
                                         "}"));

  setStyleSheet(utility::getStyleSheet("://window/window.css") + utility::getStyleSheet(":indexing_dialog/indexing_dialog.css"));

  m_layout->setContentsMargins(20, 20, 20, 0);
  m_layout->setSpacing(3);

  m_content->setLayout(m_layout);
}

void QtIndexingDialog::resizeEvent(QResizeEvent* event) {
  QSize size = event->size();

  if(size.width() < 300) {
    size.setWidth(300);
    resize(size);
    return;
  }

  if(size.height() < 200) {
    size.setHeight(200);
    resize(size);
    return;
  }

  if(!m_isSubWindow) {
    m_window->resize(size);
    return;
  }

  const QSize windowSize = size - QSize(30, 30);

  m_window->resize(windowSize);
  m_window->move(15, 15);
}

void QtIndexingDialog::setupDone() {
  const QSize actualSize = m_window->sizeHint() + QSize(50, 50);
  const QSize preferredSize = sizeHint();

  resize(QSize{qMax(actualSize.width(), preferredSize.width()), qMax(actualSize.height(), preferredSize.height())});

  moveToCenter();
}
