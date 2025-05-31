#include "QtProjectWizardContent.h"

#include <thread>

#include <QGridLayout>
#include <QLabel>
#include <QToolButton>

#include "QtTextEditDialog.h"
#include "utility.h"
#include "utilityString.h"


QtProjectWizardContent::QtProjectWizardContent(QtProjectWizardWindow* window)
    : QWidget{window}
    , mWindow{window}
    , mShowFilesFunctor{[this](auto&& PH1) { showFilesDialog(std::forward<decltype(PH1)>(PH1)); }} {}

void QtProjectWizardContent::populate(QGridLayout* /*layout*/, int& /*row**/) {}

void QtProjectWizardContent::windowReady() {}

void QtProjectWizardContent::load() {}

void QtProjectWizardContent::save() {}

void QtProjectWizardContent::refresh() {}

bool QtProjectWizardContent::check() {
  return true;
}

std::vector<FilePath> QtProjectWizardContent::getFilePaths() const {
  return {};
}

QString QtProjectWizardContent::getFileNamesTitle() const {
  return QStringLiteral("File List");
}

QString QtProjectWizardContent::getFileNamesDescription() const {
  return QStringLiteral("files");
}

bool QtProjectWizardContent::isRequired() const {
  return mIsRequired;
}

void QtProjectWizardContent::setIsRequired(bool isRequired) {
  mIsRequired = isRequired;
}

QLabel* QtProjectWizardContent::createFormTitle(const QString& name) const {
  auto* label = new QLabel{name};    // NOLINT
  label->setObjectName(QStringLiteral("titleLabel"));
  label->setWordWrap(true);
  return label;
}

QLabel* QtProjectWizardContent::createFormLabel(QString name) const {
  if(mIsRequired) {
    name += QStringLiteral("*");
  }

  return createFormSubLabel(name);
}

QLabel* QtProjectWizardContent::createFormSubLabel(const QString& name) const {
  auto* label = new QLabel{name};    // NOLINT
  label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  label->setObjectName(QStringLiteral("label"));
  label->setWordWrap(true);
  return label;
}

QToolButton* QtProjectWizardContent::createSourceGroupButton(const QString& name, const QString& iconPath) const {
  auto* button = new QToolButton;    // NOLINT
  button->setObjectName(QStringLiteral("sourceGroupButton"));
  button->setText(name);
  button->setIcon(QPixmap(iconPath));
  button->setIconSize(QSize(64, 64));
  button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  button->setCheckable(true);
  return button;
}

QtHelpButton* QtProjectWizardContent::addHelpButton(const QString& helpTitle,
                                                    const QString& helpText,
                                                    QGridLayout* layout,
                                                    int row) const {
  auto* button = new QtHelpButton{QtHelpButtonInfo(helpTitle, helpText)};    // NOLINT
  button->setMessageBoxParent(mWindow);
  layout->addWidget(button, row, QtProjectWizardWindow::HELP_COL, Qt::AlignTop);
  return button;
}

QPushButton* QtProjectWizardContent::addFilesButton(const QString& name, QGridLayout* layout, int row) const {
  auto* button = new QPushButton{name};    // NOLINT
  button->setObjectName(QStringLiteral("windowButton"));
  button->setAttribute(Qt::WA_LayoutUsesWidgetRect);    // fixes layouting on Mac
  if(nullptr != layout) {
    layout->addWidget(button, row, QtProjectWizardWindow::BACK_COL, Qt::AlignRight | Qt::AlignTop);
  }
  std::ignore = connect(button, &QPushButton::clicked, this, &QtProjectWizardContent::filesButtonClicked);

  return button;
}

QFrame* QtProjectWizardContent::addSeparator(QGridLayout* layout, int row) const {
  auto* separator = new QFrame;    // NOLINT
  separator->setFrameShape(QFrame::HLine);

  QPalette palette = separator->palette();
  palette.setColor(QPalette::WindowText, Qt::lightGray);
  separator->setPalette(palette);

  layout->addWidget(separator, row, 0, 1, -1);
  return separator;
}

void QtProjectWizardContent::filesButtonClicked() {
  mWindow->saveContent();
  mWindow->refreshContent();

  // TODO(Hussein): Remove detach
  std::thread([&]() {
    const std::vector<FilePath> filePaths = getFilePaths();
    mShowFilesFunctor(filePaths);
  }).detach();
}

void QtProjectWizardContent::showFilesDialog(const std::vector<FilePath>& filePaths) {
  if(nullptr == mFilesDialog) {
    mFilesDialog = new QtTextEditDialog(
        getFileNamesTitle(), QString::number(filePaths.size()) + " " + getFileNamesDescription(), mWindow);
    mFilesDialog->setup();

    mFilesDialog->setText(utility::join(utility::toWStrings(filePaths), L"\n"));
    mFilesDialog->setCloseVisible(false);
    mFilesDialog->setReadOnly(true);

    std::ignore = connect(mFilesDialog, &QtTextEditDialog::finished, this, &QtProjectWizardContent::closedFilesDialog);
    std::ignore = connect(mFilesDialog, &QtTextEditDialog::canceled, this, &QtProjectWizardContent::closedFilesDialog);
  }

  mFilesDialog->showWindow();
  mFilesDialog->raise();
}

void QtProjectWizardContent::closedFilesDialog() {
  mFilesDialog->hide();
  mFilesDialog->deleteLater();
  mFilesDialog = nullptr;

  window()->raise();
}
