#pragma once
#include "Bookmark.h"
#include "View.h"

class BookmarkController;

class BookmarkView : public View {
public:
  explicit BookmarkView(ViewLayout* viewLayout);
  ~BookmarkView() override;

  [[nodiscard]] std::string getName() const override;

  virtual void displayBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks) = 0;
  virtual void displayBookmarkEditor(std::shared_ptr<Bookmark> bookmark, const std::vector<BookmarkCategory>& categories) = 0;
  virtual void displayBookmarkCreator(const std::vector<std::wstring>& names,
                                      const std::vector<BookmarkCategory>& categories,
                                      Id nodeId) = 0;

  [[nodiscard]] virtual bool bookmarkBrowserIsVisible() const = 0;

private:
  BookmarkController* getController();
};