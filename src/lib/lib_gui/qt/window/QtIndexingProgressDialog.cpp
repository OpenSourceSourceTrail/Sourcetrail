#include "QtIndexingProgressDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "FilePath.h"
#include "type/indexing/MessageIndexingInterrupted.h"

QtIndexingProgressDialog::QtIndexingProgressDialog(bool /*hideable*/, QWidget* parent)
    : QtProgressBarDialog(0.38F, parent)
    , m_filePathLabel(new QLabel())
    , m_errorWidget(QtIndexingDialog::createErrorWidget(m_layout)) {
  setSizeGripStyle(false);

  updateTitle(QStringLiteral("Indexing Files"));


  m_filePathLabel->setObjectName(QStringLiteral("filePath"));
  m_filePathLabel->setAlignment(Qt::AlignRight);
  m_layout->addWidget(m_filePathLabel);

  m_layout->addSpacing(12);


  m_layout->addStretch();

  {
    auto* buttons = new QHBoxLayout();    // NOLINT(cppcoreguidelines-owning-memory)

    auto* stopButton = new QPushButton(QStringLiteral("Stop"));    // NOLINT(cppcoreguidelines-owning-memory)
    stopButton->setObjectName(QStringLiteral("windowButton"));
    connect(stopButton, &QPushButton::clicked, this, &QtIndexingProgressDialog::onStopPressed);
    buttons->addWidget(stopButton);

    buttons->addStretch();

    auto* hideButton = new QPushButton(QStringLiteral("Hide"));    // NOLINT(cppcoreguidelines-owning-memory)
    hideButton->setObjectName(QStringLiteral("windowButton"));
    hideButton->setDefault(true);
    connect(hideButton, &QPushButton::clicked, this, &QtIndexingProgressDialog::onHidePressed);
    buttons->addWidget(hideButton);

    m_layout->addLayout(buttons);
  }

  setupDone();
  setGeometries();
}

QSize QtIndexingProgressDialog::sizeHint() const {
  return {350, 350};
}

void QtIndexingProgressDialog::updateIndexingProgress(size_t fileCount, size_t totalFileCount, const FilePath& sourcePath) {
  updateMessage(QString::number(fileCount) + "/" + QString::number(totalFileCount) + " File" + (totalFileCount > 1 ? "s" : ""));

  size_t progress = 0;
  if(totalFileCount > 0) {
    progress = fileCount * 100 / totalFileCount;
  }

  if(!sourcePath.empty()) {
    m_sourcePath = QString::fromStdWString(sourcePath.wstr());
  }

  updateProgress(progress);
}

void QtIndexingProgressDialog::updateErrorCount(size_t errorCount, size_t fatalCount) {
  if((m_errorWidget != nullptr) && (errorCount != 0U)) {
    QString str = QString::number(errorCount) + " Error";
    if(errorCount > 1) {
      str += QLatin1String("s");
    }

    if(fatalCount != 0U) {
      str += " (" + QString::number(fatalCount) + " Fatal)";
    }

    auto* errorCountButton = m_errorWidget->findChild<QPushButton*>(QStringLiteral("errorCount"));
    errorCountButton->setText(str);

    m_errorWidget->show();
  }
}

void QtIndexingProgressDialog::onHidePressed() {
  emit visibleChanged(false);
}

void QtIndexingProgressDialog::onStopPressed() {
  if(isVisible()) {
    MessageIndexingInterrupted().dispatch();
  }
}

void QtIndexingProgressDialog::closeEvent(QCloseEvent* /*event*/) {
  emit visibleChanged(false);
}

void QtIndexingProgressDialog::setGeometries() {
  QtProgressBarDialog::setGeometries();

  if(m_filePathLabel != nullptr) {
    m_filePathLabel->setText(m_filePathLabel->fontMetrics().elidedText(m_sourcePath, Qt::ElideLeft, m_filePathLabel->width()));
  }
}
