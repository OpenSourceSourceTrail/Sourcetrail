#pragma once
#include "BookmarkView.h"
#include "ControllerProxy.h"
#include "QtThreadedFunctor.h"

class QFrame;
class QtBookmarkBrowser;
class BookmarkController;

class QtBookmarkView : public BookmarkView {
public:
  explicit QtBookmarkView(ViewLayout* viewLayout);
  ~QtBookmarkView() override;

  // View implementation
  void createWidgetWrapper() override;
  void refreshView() override;

  // BookmarkView implementation
  void displayBookmarkCreator(const std::vector<std::wstring>& names,
                              const std::vector<BookmarkCategory>& categories,
                              Id nodeId) override;
  void displayBookmarkEditor(std::shared_ptr<Bookmark> bookmark, const std::vector<BookmarkCategory>& categories) override;

  void displayBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks) override;
  [[nodiscard]] bool bookmarkBrowserIsVisible() const override;

private:
  ControllerProxy<BookmarkController> m_controllerProxy;
  QtThreadedLambdaFunctor m_onQtThread;

  QtBookmarkBrowser* m_bookmarkBrowser = nullptr;
};
