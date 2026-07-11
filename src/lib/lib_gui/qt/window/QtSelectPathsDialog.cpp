#include "QtSelectPathsDialog.h"

#include <set>
#include <tuple>

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "FilePath.h"
#include "utility.h"

QtSelectPathsDialog::QtSelectPathsDialog(const QString& title, const QString& description, QWidget* parent)
    : QtTextEditDialog(title, description, parent) {}

std::vector<FilePath> QtSelectPathsDialog::getPathsList() const {
  std::vector<FilePath> checkedPaths;

  for(int i = 0; i < m_list->count(); i++) {
    if(m_list->item(i)->checkState() == Qt::Checked) {
      checkedPaths.emplace_back(m_list->item(i)->text().toStdWString());
    }
  }

  return checkedPaths;
}

void QtSelectPathsDialog::setPathsList(const std::vector<FilePath>& paths,
                                       const std::vector<FilePath>& checkedPaths,
                                       const FilePath& rootPathForRelativePaths) {
  std::set<FilePath> checked(checkedPaths.begin(), checkedPaths.end());

  for(FilePath filePath : utility::unique(utility::concat(paths, checkedPaths))) {
    auto* item = new QListWidgetItem{QString::fromStdWString(filePath.wstr()), m_list};
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);    // set checkable flag

    if(checked.find(filePath) == checked.end()) {
      item->setCheckState(Qt::Unchecked);    // AND initialize check state
    } else {
      item->setCheckState(Qt::Checked);
    }

    if(!filePath.isAbsolute()) {
      filePath = rootPathForRelativePaths.getConcatenated(filePath);
    }

    if(!filePath.exists()) {
      item->setForeground(Qt::red);
      item->setToolTip(QStringLiteral("Path does not exist"));
      item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
      item->setCheckState(Qt::Unchecked);
    } else {
      item->setForeground(Qt::black);
    }
  }
}

void QtSelectPathsDialog::checkSelected(bool checked) {
  for(QListWidgetItem* item : m_list->selectedItems()) {
    item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
  }
}

void QtSelectPathsDialog::populateWindow(QWidget* widget) {
  auto* layout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  layout->setContentsMargins(0, 0, 0, 0);

  auto* description = new QLabel{m_description};    // NOLINT(cppcoreguidelines-owning-memory)
  description->setObjectName(QStringLiteral("description"));
  description->setWordWrap(true);
  layout->addWidget(description);

  m_list = new QListWidget;    // NOLINT(cppcoreguidelines-owning-memory)
  m_list->setObjectName(QStringLiteral("pathList"));
  m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_list->setAttribute(Qt::WA_MacShowFocusRect, false);
  layout->addWidget(m_list);

  auto* buttonLayout = new QHBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  buttonLayout->setContentsMargins(0, 0, 0, 0);

  auto* checkAllButton = new QPushButton{QStringLiteral("check all")};
  checkAllButton->setObjectName(QStringLiteral("windowButton"));
  std::ignore = connect(checkAllButton, &QPushButton::clicked, [this]() {
    m_list->selectAll();
    checkSelected(true);
    m_list->clearSelection();
  });
  buttonLayout->addWidget(checkAllButton);

  auto* unCheckAllButton = new QPushButton{QStringLiteral("uncheck all")};
  unCheckAllButton->setObjectName(QStringLiteral("windowButton"));
  std::ignore = connect(unCheckAllButton, &QPushButton::clicked, [this]() {
    m_list->selectAll();
    checkSelected(false);
    m_list->clearSelection();
  });
  buttonLayout->addWidget(unCheckAllButton);

  auto* checkSelectedButton = new QPushButton{QStringLiteral("check selected")};
  checkSelectedButton->setObjectName(QStringLiteral("windowButton"));
  std::ignore = connect(checkSelectedButton, &QPushButton::clicked, this, [this]() { checkSelected(true); });
  buttonLayout->addWidget(checkSelectedButton);

  auto* unCheckSelectedButton = new QPushButton{QStringLiteral("uncheck selected")};
  unCheckSelectedButton->setObjectName(QStringLiteral("windowButton"));
  std::ignore = connect(unCheckSelectedButton, &QPushButton::clicked, this, [this]() { checkSelected(false); });
  buttonLayout->addWidget(unCheckSelectedButton);

  layout->addLayout(buttonLayout);

  widget->setLayout(layout);
}

void QtSelectPathsDialog::windowReady() {
  updateNextButton(QStringLiteral("Save"));
  updateCloseButton(QStringLiteral("Cancel"));

  setPreviousVisible(false);

  updateTitle(m_title);
}
