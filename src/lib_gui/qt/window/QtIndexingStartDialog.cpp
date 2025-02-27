#include "QtIndexingStartDialog.h"

#include <tuple>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "QtHelpButton.h"

QtIndexingStartDialog::QtIndexingStartDialog(const std::vector<RefreshMode>& enabledModes,
                                             const RefreshMode initialMode,
                                             bool enabledShallowOption,
                                             bool initialShallowState,
                                             QWidget* parent)
    : QtIndexingDialog(true, parent)
    , m_clearLabel(QtIndexingDialog::createMessageLabel(m_layout))
    , m_indexLabel(QtIndexingDialog::createMessageLabel(m_layout)) {
  setSizeGripStyle(false);

  QtIndexingDialog::createTitleLabel(QStringLiteral("Start Indexing"), m_layout);
  m_layout->addSpacing(5);


  m_clearLabel->setVisible(false);
  m_indexLabel->setVisible(false);

  m_layout->addStretch();

  auto* subLayout = new QHBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  subLayout->addStretch();

  auto* modeLayout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  modeLayout->setSpacing(7);

  auto* modeTitleLayout = new QHBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  modeTitleLayout->setSpacing(7);

  QLabel* modeLabel = QtIndexingDialog::createMessageLabel(modeTitleLayout);
  modeLabel->setText(QStringLiteral("Mode:"));
  modeLabel->setAlignment(Qt::AlignLeft);

  auto* helpButton = new QtHelpButton(QtHelpButtonInfo(
      QStringLiteral("Indexing Modes"),
      QString("<b>Updated files:</b> Reindexes all files that were modified since the last "
              "indexing, all new files and all files depending "
              "on those.<br /><br />"
              "<b>Incomplete & updated files:</b> Reindexes all files that had errors during "
              "last indexing, all updated files and all files "
              "depending on those.<br /><br />"
              "<b>All files:</b> Deletes the previous index and reindexes all files from "
              "scratch.<br /><br />") +
          (enabledShallowOption ? "<br /><b>Shallow Python Indexing:</b> References within your code base (calls, "
                                  "usages, etc.) are resolved by name, which is "
                                  "imprecise but much faster than in-depth indexing.<br />"
                                  "<i>Hint: Use this option for a quick first indexing pass and start browsing "
                                  "the code base "
                                  "while running a second pass for in-depth indexing.<br /><br />" :
                                  "")));
  helpButton->setColor(Qt::white);
  modeTitleLayout->addWidget(helpButton);

  modeTitleLayout->addStretch();

  modeLayout->addLayout(modeTitleLayout);
  modeLayout->addSpacing(5);

  m_refreshModeButtons.emplace(
      RefreshMode::UpdatedFiles, new QRadioButton(QStringLiteral("Updated files")));    // NOLINT(cppcoreguidelines-owning-memory)
  m_refreshModeButtons.emplace(
      RefreshMode::UpdatedAndIncompleteFiles,
      new QRadioButton(QStringLiteral("Incomplete && updated files")));    // NOLINT(cppcoreguidelines-owning-memory)
  m_refreshModeButtons.emplace(
      RefreshMode::AllFiles, new QRadioButton(QStringLiteral("All files")));    // NOLINT(cppcoreguidelines-owning-memory)

  std::function<void(bool)> func = [this](bool checked) {
    if(!checked) {
      return;
    }

    for(const auto& [refreshMode, button] : m_refreshModeButtons) {
      if(button->isChecked()) {
        emit setMode(refreshMode);
        return;
      }
    }
  };

  for(const auto& [refreshMode, button] : m_refreshModeButtons) {
    button->setObjectName(QStringLiteral("option"));
    button->setEnabled(false);
    if(refreshMode == initialMode) {
      button->setChecked(true);
    }
    modeLayout->addWidget(button);
    std::ignore = connect(button, &QRadioButton::toggled, button, func);
  }

  for(RefreshMode mode : enabledModes) {
    m_refreshModeButtons[mode]->setEnabled(true);
  }

  if(enabledShallowOption) {
    auto* shallowIndexingCheckBox = new QCheckBox{QStringLiteral("Shallow Python Indexing")};
    std::ignore = connect(shallowIndexingCheckBox, &QCheckBox::toggled, [this, shallowIndexingCheckBox]() {
      emit setShallowIndexing(shallowIndexingCheckBox->isChecked());
    });
    shallowIndexingCheckBox->setChecked(initialShallowState);
    modeLayout->addWidget(shallowIndexingCheckBox);
  }

  subLayout->addLayout(modeLayout);
  m_layout->addLayout(subLayout);

  m_layout->addSpacing(20);

  {
    auto* buttons = new QHBoxLayout;                                   // NOLINT(cppcoreguidelines-owning-memory)
    auto* cancelButton = new QPushButton{QStringLiteral("Cancel")};    // NOLINT(cppcoreguidelines-owning-memory)
    cancelButton->setObjectName(QStringLiteral("windowButton"));
    std::ignore = connect(cancelButton, &QPushButton::clicked, this, &QtIndexingStartDialog::onCancelPressed);
    buttons->addWidget(cancelButton);

    buttons->addStretch();

    auto* startButton = new QPushButton(QStringLiteral("Start"));    // NOLINT(cppcoreguidelines-owning-memory)
    startButton->setObjectName(QStringLiteral("windowButton"));
    startButton->setDefault(true);
    connect(startButton, &QPushButton::clicked, this, &QtIndexingStartDialog::onStartPressed);
    buttons->addWidget(startButton);
    m_layout->addLayout(buttons);
  }

  setupDone();
}

QSize QtIndexingStartDialog::sizeHint() const {
  return {350, 310};
}

void QtIndexingStartDialog::updateRefreshInfo(const RefreshInfo& info) {
  QRadioButton* button = m_refreshModeButtons.find(info.mode)->second;
  if(!button->isChecked()) {
    button->setChecked(true);
  }

  size_t clearCount = info.filesToClear.size();
  size_t indexCount = info.filesToIndex.size();

  m_clearLabel->setText("Files to clear: " + QString::number(clearCount));
  m_indexLabel->setText("Source files to index: " + QString::number(indexCount));

  m_clearLabel->setVisible((clearCount != 0U) && RefreshMode::AllFiles != info.mode);
  m_indexLabel->setVisible(true);
}

void QtIndexingStartDialog::resizeEvent(QResizeEvent* event) {
  QtIndexingDialog::resizeEvent(event);
}

void QtIndexingStartDialog::closeEvent(QCloseEvent* /*event*/) {
  emit QtIndexingDialog::canceled();
}

void QtIndexingStartDialog::keyPressEvent(QKeyEvent* event) {
  switch(event->key()) {
  case Qt::Key_Escape:
    onCancelPressed();
    break;
  case Qt::Key_Return:
    onStartPressed();
    break;
  }

  QWidget::keyPressEvent(event);
}

void QtIndexingStartDialog::onStartPressed() {
  for(const auto& [mode, button] : m_refreshModeButtons) {
    if(nullptr != button && button->isChecked()) {
      emit startIndexing(mode);
      return;
    }
  }

  emit finished();
}

void QtIndexingStartDialog::onCancelPressed() {
  emit canceled();
}
