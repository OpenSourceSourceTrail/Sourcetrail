#pragma once
#include "ActivationListener.h"
#include "Bookmark.h"
#include "Controller.h"
#include "EdgeBookmark.h"
#include "type/bookmark/MessageBookmarkActivate.h"
#include "type/bookmark/MessageBookmarkBrowse.h"
#include "type/bookmark/MessageBookmarkCreate.h"
#include "type/bookmark/MessageBookmarkDelete.h"
#include "type/bookmark/MessageBookmarkEdit.h"
#include "type/indexing/MessageIndexingFinished.h"
#include "MessageListener.h"
#include "NodeBookmark.h"

class StorageAccess;

class BookmarkController
    : public Controller
    , public ActivationListener
    , public MessageListener<MessageBookmarkActivate>
    , public MessageListener<MessageBookmarkBrowse>
    , public MessageListener<MessageBookmarkCreate>
    , public MessageListener<MessageBookmarkDelete>
    , public MessageListener<MessageBookmarkEdit>
    , public MessageListener<MessageIndexingFinished> {
public:
  explicit BookmarkController(StorageAccess* storageAccess);
  ~BookmarkController() override;

  void clear() override;

  void displayBookmarks();
  void displayBookmarksFor(Bookmark::Filter filter, Bookmark::Order order);

  void createBookmark(const std::wstring& name, const std::wstring& comment, const std::wstring& category, GlobalId nodeId);
  void editBookmark(Id bookmarkId, const std::wstring& name, const std::wstring& comment, const std::wstring& category);

  void deleteBookmark(Id bookmarkId);
  void deleteBookmarkCategory(Id categoryId);
  void deleteBookmarkForActiveTokens();

  void activateBookmark(const std::shared_ptr<Bookmark> bookmark);

  void showBookmarkCreator(Id nodeId = 0);
  void showBookmarkEditor(const std::shared_ptr<Bookmark> bookmark);

private:
  class BookmarkCache {
  public:
    BookmarkCache(StorageAccess* storageAccess);

    void clear();

    std::vector<NodeBookmark> getAllNodeBookmarks();
    std::vector<EdgeBookmark> getAllEdgeBookmarks();

  private:
    StorageAccess* m_storageAccess;
    std::vector<NodeBookmark> m_nodeBookmarks;
    std::vector<EdgeBookmark> m_edgeBookmarks;
    bool m_nodeBookmarksValid;
    bool m_edgeBookmarksValid;
  };

  void handleActivation(const MessageActivateBase* message) override;

  void handleMessage(MessageActivateTokens* message) override;
  void handleMessage(MessageBookmarkActivate* message) override;
  void handleMessage(MessageBookmarkBrowse* message) override;
  void handleMessage(MessageBookmarkCreate* message) override;
  void handleMessage(MessageBookmarkDelete* message) override;
  void handleMessage(MessageBookmarkEdit* message) override;
  void handleMessage(MessageIndexingFinished* message) override;

  [[nodiscard]] std::vector<std::wstring> getActiveTokenDisplayNames() const;
  [[nodiscard]] std::vector<std::wstring> getDisplayNamesForNodeId(Id nodeId) const;

  [[nodiscard]] std::vector<BookmarkCategory> getAllBookmarkCategories() const;

  [[nodiscard]] std::shared_ptr<Bookmark> getBookmarkForActiveToken(Id tabId) const;
  [[nodiscard]] std::shared_ptr<Bookmark> getBookmarkForNodeId(Id nodeId) const;

  [[nodiscard]] bool canCreateBookmark() const;

  [[nodiscard]] std::vector<std::shared_ptr<Bookmark>> getAllBookmarks() const;
  [[nodiscard]] std::vector<std::shared_ptr<NodeBookmark>> getAllNodeBookmarks() const;
  [[nodiscard]] std::vector<std::shared_ptr<EdgeBookmark>> getAllEdgeBookmarks() const;
  [[nodiscard]] std::vector<std::shared_ptr<Bookmark>> getBookmarks(Bookmark::Filter filter, Bookmark::Order order) const;

  [[nodiscard]] std::vector<std::wstring> getActiveNodeDisplayNames() const;
  [[nodiscard]] std::vector<std::wstring> getActiveEdgeDisplayNames() const;
  [[nodiscard]] std::wstring getNodeDisplayName(const Id id) const;

  [[nodiscard]] std::vector<std::shared_ptr<Bookmark>> getFilteredBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks,
                                                                            Bookmark::Filter filter) const;
  [[nodiscard]] std::vector<std::shared_ptr<Bookmark>> getOrderedBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks,
                                                                           Bookmark::Order order) const;
  [[nodiscard]] std::vector<std::shared_ptr<Bookmark>> getDateOrderedBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks,
                                                                               const bool ascending) const;
  [[nodiscard]] std::vector<std::shared_ptr<Bookmark>> getNameOrderedBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks,
                                                                               const bool ascending) const;

  void cleanBookmarkCategories();

  static bool bookmarkDateCompare(const std::shared_ptr<Bookmark>& a, const std::shared_ptr<Bookmark>& b);
  static bool bookmarkNameCompare(const std::shared_ptr<Bookmark>& a, const std::shared_ptr<Bookmark>& b);

  void update();

  static const std::wstring sEdgeSeparatorToken;
  static const std::wstring sDefaultCategoryName;

  StorageAccess* const mStorageAccess;
  mutable BookmarkCache mBookmarkCache;

  mutable std::map<Id, std::vector<Id>> mActiveNodeIds;
  mutable std::map<Id, std::vector<Id>> mActiveEdgeIds;

  Bookmark::Filter mFilter = Bookmark::Filter::All;
  Bookmark::Order mOrder = Bookmark::Order::DateDescending;
};