#pragma once
#include <QComboBox>
#include <QListWidget>
#include <QTreeWidget>

#include "Bookmark.h"
#include "BookmarkController.h"
#include "ControllerProxy.h"
#include "QtWindow.h"

class QtBookmark;

class QtBookmarkBrowser : public QtWindow {
  Q_OBJECT

public:
  explicit QtBookmarkBrowser(ControllerProxy<BookmarkController>* controllerProxy, QWidget* parent = nullptr);
  ~QtBookmarkBrowser() override;

  void setupBookmarkBrowser();
  void setBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks);

protected:
  void resizeEvent(QResizeEvent* event) override;
  void handleClose() override;
  void handleNext() override;

private slots:
  void filterOrOrderChanged(int index);
  void treeItemClicked(QTreeWidgetItem* item, int column);

private:
  Bookmark::Filter getSelectedFilter();
  Bookmark::Order getSelectedOrder();

  QTreeWidgetItem* findOrCreateTreeCategory(const BookmarkCategory& category);

  ControllerProxy<BookmarkController>* mControllerProxy;

  QTreeWidget* mBookmarkTree = nullptr;

  QComboBox* mFilterComboBox = nullptr;
  QComboBox* mOrderComboBox = nullptr;
  std::vector<std::string> mOrderNames;

  QWidget* mHeaderBackground = nullptr;
};