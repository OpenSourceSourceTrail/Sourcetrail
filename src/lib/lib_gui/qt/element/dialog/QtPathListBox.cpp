#include "QtPathListBox.h"

#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QtGui/qevent.h>

#include "QtPathListBoxItem.h"
#include "utilityFile.h"

QtPathListBox::QtPathListBox(QWidget* parent, const QString& listName, SelectionPolicyType selectionPolicy)
    : QtListBox(parent, listName), m_selectionPolicy(selectionPolicy) {
  auto* dropInfoText = new QLabel(QStringLiteral("Drop Files & Folders"));
  dropInfoText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  dropInfoText->setObjectName(QStringLiteral("dropInfo"));
  dropInfoText->setAlignment(Qt::AlignRight);

  addWidgetToBar(dropInfoText);
}

QtPathListBox::SelectionPolicyType QtPathListBox::getSelectionPolicy() const {
  return m_selectionPolicy;
}

const FilePath& QtPathListBox::getRelativeRootDirectory() const {
  return m_relativeRootDirectory;
}

void QtPathListBox::setRelativeRootDirectory(const FilePath& dir) {
  m_relativeRootDirectory = dir;
}

std::vector<FilePath> QtPathListBox::getPathsAsDisplayed() const {
  std::vector<FilePath> paths;
  for(int i = 0; i < m_list->count(); ++i) {
    auto* widget = dynamic_cast<QtListBoxItem*>(m_list->itemWidget(m_list->item(i)));
    paths.emplace_back(widget->getText().toStdWString());
  }
  return paths;
}

std::vector<FilePath> QtPathListBox::getPathsAsAbsolute() const {
  std::vector<FilePath> paths = getPathsAsDisplayed();
  for(FilePath& path : paths) {
    makeAbsolute(path);
  }
  return paths;
}

void QtPathListBox::setPaths(const std::vector<FilePath>& list, bool readOnly) {
  clear();
  addPaths(list, readOnly);
}

void QtPathListBox::addPaths(const std::vector<FilePath>& list, bool readOnly) {
  for(const FilePath& path : list) {
    QtListBoxItem* item = addListBoxItemWithText(QString::fromStdWString(path.wstr()));
    item->setReadOnly(readOnly);
  }
}

void QtPathListBox::makeAbsolute(FilePath& path) const {
  if(!path.empty() && !path.isAbsolute() && !m_relativeRootDirectory.empty()) {
    path = m_relativeRootDirectory.getConcatenated(path);
  }
}

void QtPathListBox::makeRelativeIfShorter(FilePath& path) const {
  path = utility::getAsRelativeIfShorter(path, m_relativeRootDirectory);
}

void QtPathListBox::dropEvent(QDropEvent* event) {
  foreach(QUrl url, event->mimeData()->urls()) {
    FilePath path(url.toLocalFile().toStdWString());
    makeRelativeIfShorter(path);
    addListBoxItemWithText(QString::fromStdWString(path.wstr()));
  }
}

void QtPathListBox::dragEnterEvent(QDragEnterEvent* event) {
  event->accept();
}

QtListBoxItem* QtPathListBox::createListBoxItem(QListWidgetItem* item) {
  return new QtPathListBoxItem(this, item);
}
