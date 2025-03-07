#include "QtContextMenu.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>

#include "logging.h"
#include "type/history/MessageHistoryRedo.h"
#include "type/history/MessageHistoryUndo.h"

QtContextMenu* QtContextMenu::sInstance = nullptr;

QAction* QtContextMenu::sUndoAction = nullptr;
QAction* QtContextMenu::sRedoAction = nullptr;

QAction* QtContextMenu::sCopyFullPathAction = nullptr;
QAction* QtContextMenu::sOpenContainingFolderAction = nullptr;

FilePath QtContextMenu::sFilePath;

QtContextMenu::QtContextMenu(const QContextMenuEvent* event, QWidget* origin) : mMenu(origin), mPoint(event->globalPos()) {
  getInstance();
}

QtContextMenu::QtContextMenu() = default;

QtContextMenu::~QtContextMenu() = default;

void QtContextMenu::addAction(QAction* action) {
  mMenu.addAction(action);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void QtContextMenu::enableUndo(bool enabled) {
  sUndoAction->setEnabled(enabled);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void QtContextMenu::enableRedo(bool enabled) {
  sRedoAction->setEnabled(enabled);
}

void QtContextMenu::addUndoActions() {
  addAction(sUndoAction);
  addAction(sRedoAction);
}

void QtContextMenu::addFileActions(const FilePath& filePath) {
  sFilePath = filePath;

  sCopyFullPathAction->setEnabled(!sFilePath.empty());
  sOpenContainingFolderAction->setEnabled(!sFilePath.empty());

  addAction(sCopyFullPathAction);
  addAction(sOpenContainingFolderAction);
}

QtContextMenu* QtContextMenu::getInstance() {
  if(nullptr != sInstance) {
    return sInstance;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles it
  sInstance = new QtContextMenu;

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles it
  sUndoAction = new QAction(tr("Back"), sInstance);
  sUndoAction->setStatusTip(tr("Go back to last active symbol"));
  sUndoAction->setToolTip(tr("Go back to last active symbol"));
  connect(sUndoAction, &QAction::triggered, sInstance, &QtContextMenu::undoActionTriggered);

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles it
  sRedoAction = new QAction(tr("Forward"), sInstance);
  sRedoAction->setStatusTip(tr("Go forward to next active symbol"));
  sRedoAction->setToolTip(tr("Go forward to next active symbol"));
  connect(sRedoAction, &QAction::triggered, sInstance, &QtContextMenu::redoActionTriggered);

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles it
  sCopyFullPathAction = new QAction(tr("Copy Full Path"), sInstance);
  sCopyFullPathAction->setStatusTip(tr("Copies the path of this file to the clipboard"));
  sCopyFullPathAction->setToolTip(tr("Copies the path of this file to the clipboard"));
  connect(sCopyFullPathAction, &QAction::triggered, sInstance, &QtContextMenu::copyFullPathActionTriggered);

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles it
  sOpenContainingFolderAction = new QAction(tr("Open Containing Folder"), sInstance);
  sOpenContainingFolderAction->setStatusTip(tr("Opens the folder that contains this file"));
  sOpenContainingFolderAction->setToolTip(tr("Opens the folder that contains this file"));
  connect(sOpenContainingFolderAction, &QAction::triggered, sInstance, &QtContextMenu::openContainingFolderActionTriggered);

  return sInstance;
}

void QtContextMenu::addSeparator() {
  mMenu.addSeparator();
}

void QtContextMenu::show() {
  mMenu.exec(mPoint);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void QtContextMenu::undoActionTriggered() {
  MessageHistoryUndo{}.dispatch();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void QtContextMenu::redoActionTriggered() {
  MessageHistoryRedo{}.dispatch();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void QtContextMenu::copyFullPathActionTriggered() {
  QApplication::clipboard()->setText(QDir::toNativeSeparators(QString::fromStdWString(sFilePath.wstr())));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void QtContextMenu::openContainingFolderActionTriggered() {
  const auto dir = sFilePath.getParentDirectory();
  if(dir.exists()) {
    QDesktopServices::openUrl(QUrl(QString::fromStdWString(L"file:///" + dir.wstr()), QUrl::TolerantMode));
  } else {
    LOG_ERROR(L"Unable to open directory: {}", dir.wstr());
  }
}