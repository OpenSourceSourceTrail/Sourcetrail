#include "QtProgressBarDialog.h"

#include <QLabel>
#include <QVBoxLayout>

#include "QtProgressBar.h"

QtProgressBarDialog::QtProgressBarDialog(float topRatio, QWidget* parent)
    : QtIndexingDialog(true, parent)
    , m_title{new QLabel(QLatin1String(""), this)}
    , m_top{new QWidget(m_window)}
    , m_topRatio(topRatio)
    , m_progressBar{new QtProgressBar(m_window)}
    , m_percentLabel{new QLabel(QStringLiteral("0% Progress"))}
    , m_messageLabel(QtIndexingDialog::createMessageLabel(m_layout)) {
  m_top->setObjectName(QStringLiteral("topHalf"));
  m_top->setGeometry(0, 0, 0, 0);
  m_top->show();
  m_top->lower();

  m_progressBar->setGeometry(0, 0, 0, 0);

  m_title->setObjectName(QStringLiteral("title"));
  m_title->setAlignment(Qt::AlignRight | Qt::AlignBottom);
  m_title->show();

  m_percentLabel->setObjectName(QStringLiteral("percent"));
  m_layout->addWidget(m_percentLabel, 0, Qt::AlignRight);
}

void QtProgressBarDialog::updateTitle(const QString& title) {
  if(m_title != nullptr) {
    m_title->setText(title);
  }
}

std::wstring QtProgressBarDialog::getTitle() const {
  if(m_title != nullptr) {
    return m_title->text().toStdWString();
  }
  return L"";
}

void QtProgressBarDialog::updateMessage(const QString& message) {
  if(m_messageLabel != nullptr) {
    m_messageLabel->setText(message);
  }
}

std::wstring QtProgressBarDialog::getMessage() const {
  return m_messageLabel != nullptr ? m_messageLabel->text().toStdWString() : L"";
}

void QtProgressBarDialog::setUnknownProgress() {
  m_progressBar->showUnknownProgressAnimated();
  m_percentLabel->hide();
}

void QtProgressBarDialog::updateProgress(size_t progress) {
  size_t percent = std::min<size_t>(std::max<size_t>(progress, 0), 100);

  m_progressBar->showProgress(percent);
  m_percentLabel->setText(QString::number(percent) + "% Progress");
  m_percentLabel->show();
  setGeometries();
}

size_t QtProgressBarDialog::getProgress() const {
  return m_progressBar->getProgress();
}

void QtProgressBarDialog::resizeEvent(QResizeEvent* event) {
  QtIndexingDialog::resizeEvent(event);
  setGeometries();
}

void QtProgressBarDialog::setGeometries() {
  const auto width = static_cast<float>(m_window->size().width());
  const auto height = static_cast<float>(m_window->size().height());
  if(m_top != nullptr) {
    QMargins margins = m_content->layout()->contentsMargins();
    margins.setTop(static_cast<int>(height * m_topRatio + 10.0F));
    m_content->layout()->setContentsMargins(margins);

    m_top->setGeometry(0, 0, m_window->size().width(), static_cast<int>(height * m_topRatio));

    m_title->setGeometry(45, static_cast<int>(height * m_topRatio - 50.0F), static_cast<int>(width - 60.0F), 40);
  }

  if(m_progressBar != nullptr) {
    m_progressBar->setGeometry(0, static_cast<int>(height * m_topRatio - 5.0F), m_window->size().width(), 10);
  }
}
