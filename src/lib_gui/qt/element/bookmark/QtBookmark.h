#pragma once
#include <QFrame>

#include "ControllerProxy.h"

class QLabel;
class QPushButton;
class QTreeWidgetItem;

class Bookmark;
class BookmarkController;

class QtBookmark : public QFrame {
  Q_OBJECT

public:
  explicit QtBookmark(ControllerProxy<BookmarkController>* controllerProxy);
  ~QtBookmark() override;

  void setBookmark(std::shared_ptr<Bookmark> bookmark);
  [[nodiscard]] Id getBookmarkId() const;

  [[nodiscard]] QTreeWidgetItem* getTreeWidgetItem() const;
  void setTreeWidgetItem(QTreeWidgetItem* treeWidgetItem);

public slots:
  void commentToggled();

protected:
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
  void enterEvent(QEnterEvent* /*event*/) override;
#else
  void enterEvent(QEvent* event) override;
#endif
  void leaveEvent(QEvent* event) override;

private slots:
  void activateClicked();
  void editClicked();
  void deleteClicked();
  void elideButtonText();

private:
  void updateArrow();

  [[nodiscard]] std::string getDateString() const;

  ControllerProxy<BookmarkController>* m_controllerProxy = nullptr;

  QPushButton* m_activateButton = nullptr;
  QPushButton* m_editButton = nullptr;
  QPushButton* m_deleteButton = nullptr;
  QPushButton* m_toggleCommentButton = nullptr;

  QLabel* m_comment = nullptr;
  QLabel* m_dateLabel = nullptr;

  std::shared_ptr<Bookmark> m_bookmark;

  // pointer to the bookmark category item in the treeView, allows to refresh tree view when a
  // node changes in size (e.g. toggle comment). Not a nice solution to the problem, but couldn't
  // find anything better yet. (sizeHintChanged signal can't be emitted here...)
  QTreeWidgetItem* m_treeWidgetItem = nullptr;

  std::wstring m_arrowImageName;
  bool m_hovered = false;

  bool m_ignoreNextResize = false;
};
