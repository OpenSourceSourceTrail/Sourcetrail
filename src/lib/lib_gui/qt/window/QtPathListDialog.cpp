#include "QtPathListDialog.h"

#include <utility>

#include <QLabel>
#include <QVBoxLayout>

QtPathListDialog::QtPathListDialog(QString title,
                                   QString description,
                                   QtPathListBox::SelectionPolicyType selectionPolicy,
                                   QWidget* parent)
    : QtWindow(false, parent)
    , m_title(std::move(title))
    , m_description(std::move(description))
    , m_selectionPolicy(selectionPolicy) {}

QSize QtPathListDialog::sizeHint() const {
  return {550, 350};
}

void QtPathListDialog::setRelativeRootDirectory(const FilePath& dir) {
  m_pathList->setRelativeRootDirectory(dir);
}

void QtPathListDialog::setPaths(const std::vector<FilePath>& paths, bool readOnly) {
  m_pathList->setPaths(paths, readOnly);
}

std::vector<FilePath> QtPathListDialog::getPaths() {
  return m_pathList->getPathsAsDisplayed();
}

void QtPathListDialog::populateWindow(QWidget* widget) {
  auto* layout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  layout->setContentsMargins(0, 0, 0, 0);

  auto* description = new QLabel(m_description);    // NOLINT(cppcoreguidelines-owning-memory)
  description->setObjectName(QStringLiteral("description"));
  description->setWordWrap(true);
  layout->addWidget(description);

  layout->addSpacing(10);

  m_pathList = new QtPathListBox(nullptr, m_title, m_selectionPolicy);    // NOLINT(cppcoreguidelines-owning-memory)
  m_pathList->setMaximumHeight(1000);
  layout->addWidget(m_pathList);

  widget->setLayout(layout);
}

void QtPathListDialog::windowReady() {
  updateNextButton(QStringLiteral("Save"));
  updateCloseButton(QStringLiteral("Cancel"));

  setPreviousVisible(false);

  updateTitle(m_title);
}
