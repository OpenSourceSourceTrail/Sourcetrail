#pragma once
#include <QMenu>
#include <QObject>

#include "FilePath.h"

class QAction;
class QWidget;

class QtContextMenu final : public QObject {
  Q_OBJECT

public:
  QtContextMenu(const QContextMenuEvent* event, QWidget* origin);

  Q_DISABLE_COPY_MOVE(QtContextMenu)

  ~QtContextMenu() override;

  void addAction(QAction* action);

  void addUndoActions();

  void addFileActions(const FilePath& filePath);

  static QtContextMenu* getInstance();

  void addSeparator();

  void show();

  void enableUndo(bool enabled);

  void enableRedo(bool enabled);

private:
  QtContextMenu();

  // slots:
  void undoActionTriggered();

  void redoActionTriggered();

  void copyFullPathActionTriggered();

  void openContainingFolderActionTriggered();

  static QtContextMenu* sInstance;

  static QAction* sUndoAction;
  static QAction* sRedoAction;

  static QAction* sCopyFullPathAction;
  static QAction* sOpenContainingFolderAction;

  static FilePath sFilePath;

  QMenu mMenu;
  QPoint mPoint;
};