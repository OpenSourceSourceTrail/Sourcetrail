#include "BookmarkController.h"

#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view/transform.hpp>

#include "Bookmark.h"
#include "BookmarkView.h"
#include "EdgeBookmark.h"
#include "logging.h"
#include "NodeBookmark.h"
#include "StorageAccess.h"
#include "StorageEdge.h"
#include "TabId.h"
#include "type/bookmark/MessageBookmarkButtonState.h"
#include "type/bookmark/MessageBookmarkUpdate.hpp"
#include "type/graph/MessageActivateEdge.h"
#include "type/graph/MessageActivateNodes.h"
#include "utility.h"
#include "utilityString.h"

const std::wstring BookmarkController::sEdgeSeparatorToken = L" => ";
const std::wstring BookmarkController::sDefaultCategoryName = L"default";

BookmarkController::BookmarkController(StorageAccess* storageAccess)
    : mStorageAccess(storageAccess), mBookmarkCache(storageAccess) {
  assert(storageAccess != nullptr);
}

BookmarkController::~BookmarkController() = default;

void BookmarkController::clear() {
  mActiveNodeIds.clear();
  mActiveEdgeIds.clear();
}

void BookmarkController::displayBookmarks() {
  BookmarkView* view = getView<BookmarkView>();
  if(view == nullptr) {
    LOG_WARNING("Failed to get view");
    return;
  }
  view->displayBookmarks(getBookmarks(mFilter, mOrder));
}

void BookmarkController::displayBookmarksFor(Bookmark::Filter filter, Bookmark::Order order) {
  if(filter != Bookmark::Filter::Unknown) {
    mFilter = filter;
  }

  if(order != Bookmark::Order::None) {
    mOrder = order;
  }

  displayBookmarks();
}

void BookmarkController::createBookmark(const std::wstring& name,
                                        const std::wstring& comment,
                                        const std::wstring& category,
                                        GlobalId nodeId) {
  LOG_INFO_W(fmt::format(L"Attempting to create new bookmark ({}, {}, {})", name, comment, category));

  const GlobalId tabId = TabId::currentTab();

  const BookmarkCategory bookmarkCategory(0, category.empty() ? sDefaultCategoryName : category);

  if(const auto activeEdgeIds = mActiveEdgeIds[tabId]; activeEdgeIds.empty()) {
    LOG_INFO("Creating Node Bookmark");

    NodeBookmark bookmark(0, name, comment, TimeStamp::now(), bookmarkCategory);
    if(nodeId) {
      bookmark.addNodeId(nodeId);
    } else {
      bookmark.setNodeIds(mActiveNodeIds[tabId]);
    }

    std::ignore = mStorageAccess->addNodeBookmark(bookmark);
  } else {
    LOG_INFO("Creating Edge Bookmark");

    EdgeBookmark bookmark(0, name, comment, TimeStamp::now(), bookmarkCategory);
    bookmark.setEdgeIds(activeEdgeIds);

    if(mActiveNodeIds[tabId].empty()) {
      LOG_ERROR("Cannot create bookmark for edge if no active node exists");
    } else {
      bookmark.setActiveNodeId(activeEdgeIds.front());
    }

    std::ignore = mStorageAccess->addEdgeBookmark(bookmark);
  }

  mBookmarkCache.clear();

  if(!nodeId || (mActiveNodeIds[tabId].size() == 1 && mActiveNodeIds[tabId].front() == nodeId)) {
    MessageBookmarkButtonState(tabId, MessageBookmarkButtonState::ALREADY_CREATED).dispatch();
  }

  update();
}

void BookmarkController::editBookmark(Id bookmarkId,
                                      const std::wstring& name,
                                      const std::wstring& comment,
                                      const std::wstring& category) {
  LOG_INFO(fmt::format("Attempting to update Bookmark {}", bookmarkId));

  mStorageAccess->updateBookmark(bookmarkId, name, comment, category.size() ? category : sDefaultCategoryName);

  cleanBookmarkCategories();

  update();
}

void BookmarkController::deleteBookmark(Id bookmarkId) {
  LOG_INFO(fmt::format("Attempting to delete Bookmark {}", bookmarkId));

  mStorageAccess->removeBookmark(bookmarkId);

  cleanBookmarkCategories();

  if(!getBookmarkForActiveToken(TabId::currentTab())) {
    MessageBookmarkButtonState(TabId::currentTab(), MessageBookmarkButtonState::CAN_CREATE).dispatch();
  }

  update();
}

void BookmarkController::deleteBookmarkCategory(Id categoryId) {
  mStorageAccess->removeBookmarkCategory(categoryId);

  mBookmarkCache.clear();

  if(!getBookmarkForActiveToken(TabId::currentTab())) {
    MessageBookmarkButtonState(TabId::currentTab(), MessageBookmarkButtonState::CAN_CREATE).dispatch();
  }

  update();
}

void BookmarkController::deleteBookmarkForActiveTokens() {
  if(std::shared_ptr<Bookmark> bookmark = getBookmarkForActiveToken(TabId::currentTab())) {
    LOG_INFO_W(L"Deleting bookmark " + bookmark->getName());

    mStorageAccess->removeBookmark(bookmark->getId());

    cleanBookmarkCategories();

    MessageBookmarkButtonState(TabId::currentTab(), MessageBookmarkButtonState::CAN_CREATE).dispatch();
    update();
  } else {
    LOG_WARNING("No Bookmark to delete for active tokens.");
  }
}

void BookmarkController::activateBookmark(const std::shared_ptr<Bookmark> bookmark) {
  LOG_INFO("Attempting to activate Bookmark");

  if(std::shared_ptr<EdgeBookmark> edgeBookmark = std::dynamic_pointer_cast<EdgeBookmark>(bookmark)) {
    if(!edgeBookmark->getEdgeIds().empty()) {
      const Id firstEdgeId = edgeBookmark->getEdgeIds().front();
      const StorageEdge storageEdge = mStorageAccess->getEdgeById(firstEdgeId);

      const NameHierarchy sourceName = mStorageAccess->getNameHierarchyForNodeId(storageEdge.sourceNodeId);
      const NameHierarchy targetName = mStorageAccess->getNameHierarchyForNodeId(storageEdge.targetNodeId);

      if(edgeBookmark->getEdgeIds().size() == 1) {
        Id activeNodeId = edgeBookmark->getActiveNodeId();
        if(activeNodeId) {
          MessageActivateNodes(activeNodeId).dispatch();
        }

        MessageActivateEdge(firstEdgeId, Edge::intToType(storageEdge.type), sourceName, targetName).dispatch();
      } else {
        MessageActivateEdge activateEdge(0, Edge::EdgeType::EDGE_BUNDLED_EDGES, sourceName, targetName);
        for(const Id bundledEdgeId : edgeBookmark->getEdgeIds()) {
          activateEdge.bundledEdgesIds.push_back(bundledEdgeId);
        }
        activateEdge.dispatch();
      }
    } else {
      LOG_ERROR("Failed to activate bookmark, did not find edges to activate");
    }
  } else if(std::shared_ptr<NodeBookmark> nodeBookmark = std::dynamic_pointer_cast<NodeBookmark>(bookmark)) {
    MessageActivateNodes activateNodes;

    for(Id nodeId : nodeBookmark->getNodeIds()) {
      activateNodes.addNode(nodeId);
    }

    activateNodes.dispatch();
  }
}

void BookmarkController::showBookmarkCreator(Id nodeId) {
  const Id tabId = TabId::currentTab();
  if(!mActiveNodeIds[tabId].size() && !mActiveEdgeIds[tabId].size() && !nodeId) {
    return;
  }

  BookmarkView* view = getView<BookmarkView>();
  if(view == nullptr) {
    LOG_WARNING("Failed to get view");
    return;
  }

  if(nodeId) {
    std::shared_ptr<Bookmark> bookmark = getBookmarkForNodeId(nodeId);
    if(bookmark) {
      view->displayBookmarkEditor(bookmark, getAllBookmarkCategories());
    } else {
      view->displayBookmarkCreator(getDisplayNamesForNodeId(nodeId), getAllBookmarkCategories(), nodeId);
    }
  } else {
    std::shared_ptr<Bookmark> bookmark = getBookmarkForActiveToken(tabId);
    if(bookmark != nullptr) {
      view->displayBookmarkEditor(bookmark, getAllBookmarkCategories());
    } else {
      view->displayBookmarkCreator(getActiveTokenDisplayNames(), getAllBookmarkCategories(), 0);
    }
  }
}

void BookmarkController::showBookmarkEditor(const std::shared_ptr<Bookmark> bookmark) {
  BookmarkView* view = getView<BookmarkView>();
  if(view == nullptr) {
    LOG_WARNING("Failed to get view");
    return;
  }
  view->displayBookmarkEditor(bookmark, getAllBookmarkCategories());
}

BookmarkController::BookmarkCache::BookmarkCache(StorageAccess* storageAccess) : m_storageAccess(storageAccess) {}

void BookmarkController::BookmarkCache::clear() {
  m_nodeBookmarksValid = false;
  m_edgeBookmarksValid = false;
}

std::vector<NodeBookmark> BookmarkController::BookmarkCache::getAllNodeBookmarks() {
  if(!m_nodeBookmarksValid) {
    m_nodeBookmarks = m_storageAccess->getAllNodeBookmarks();
    m_nodeBookmarksValid = true;
  }
  return m_nodeBookmarks;
}

std::vector<EdgeBookmark> BookmarkController::BookmarkCache::getAllEdgeBookmarks() {
  if(!m_edgeBookmarksValid) {
    m_edgeBookmarks = m_storageAccess->getAllEdgeBookmarks();
    m_edgeBookmarksValid = true;
  }
  return m_edgeBookmarks;
}

void BookmarkController::handleActivation(const MessageActivateBase* /*message*/) {
  clear();
}

void BookmarkController::handleMessage(MessageActivateTokens* message) {
  Id tabId = message->getSchedulerId();
  mActiveEdgeIds[tabId].clear();

  if(message->isEdge || message->isBundledEdges) {
    mActiveEdgeIds[tabId] = message->tokenIds;

    if(getBookmarkForActiveToken(tabId)) {
      MessageBookmarkButtonState(tabId, MessageBookmarkButtonState::ALREADY_CREATED).dispatch();
    } else {
      MessageBookmarkButtonState(tabId, MessageBookmarkButtonState::CAN_CREATE).dispatch();
    }
  } else if(!message->isEdge) {
    mActiveNodeIds[tabId] = message->tokenIds;

    if(getBookmarkForActiveToken(tabId)) {
      MessageBookmarkButtonState(tabId, MessageBookmarkButtonState::ALREADY_CREATED).dispatch();
    } else {
      MessageBookmarkButtonState(tabId, MessageBookmarkButtonState::CAN_CREATE).dispatch();
    }
  }
}

void BookmarkController::handleMessage(MessageBookmarkActivate* message) {
  activateBookmark(message->bookmark);
}

void BookmarkController::handleMessage(MessageBookmarkBrowse* message) {
  displayBookmarksFor(message->filter, message->order);
}

void BookmarkController::handleMessage(MessageBookmarkCreate* message) {
  showBookmarkCreator(message->nodeId);
}

void BookmarkController::handleMessage(MessageBookmarkDelete* /*message*/) {
  deleteBookmarkForActiveTokens();
}

void BookmarkController::handleMessage(MessageBookmarkEdit* /*message*/) {
  showBookmarkCreator(0);
}

void BookmarkController::handleMessage(MessageIndexingFinished* /*message*/) {
  mBookmarkCache.clear();

  update();
}

std::vector<std::wstring> BookmarkController::getActiveTokenDisplayNames() const {
  if(mActiveEdgeIds[TabId::currentTab()].size() > 0) {
    return getActiveEdgeDisplayNames();
  } else {
    return getActiveNodeDisplayNames();
  }
}

std::vector<std::wstring> BookmarkController::getDisplayNamesForNodeId(Id nodeId) const {
  return std::vector<std::wstring>({getNodeDisplayName(nodeId)});
}

std::vector<BookmarkCategory> BookmarkController::getAllBookmarkCategories() const {
  return mStorageAccess->getAllBookmarkCategories();
}

std::shared_ptr<Bookmark> BookmarkController::getBookmarkForActiveToken(Id tabId) const {
  if(!mActiveEdgeIds[tabId].empty()) {
    for(const std::shared_ptr<EdgeBookmark>& edgeBookmark : getAllEdgeBookmarks()) {
      if(!mActiveNodeIds[tabId].empty() && edgeBookmark->getActiveNodeId() == mActiveNodeIds[tabId].front() &&
         utility::isPermutation(edgeBookmark->getEdgeIds(), mActiveEdgeIds[tabId])) {
        return std::make_shared<EdgeBookmark>(*(edgeBookmark.get()));
      }
    }
  } else {
    for(const std::shared_ptr<NodeBookmark>& nodeBookmark : getAllNodeBookmarks()) {
      if(utility::isPermutation(nodeBookmark->getNodeIds(), mActiveNodeIds[tabId])) {
        return std::make_shared<NodeBookmark>(*(nodeBookmark.get()));
      }
    }
  }

  return std::shared_ptr<Bookmark>();
}

std::shared_ptr<Bookmark> BookmarkController::getBookmarkForNodeId(Id nodeId) const {
  for(const std::shared_ptr<NodeBookmark>& nodeBookmark : getAllNodeBookmarks()) {
    if(nodeBookmark->getNodeIds().size() == 1 && nodeBookmark->getNodeIds()[0] == nodeId) {
      return std::make_shared<NodeBookmark>(*(nodeBookmark.get()));
    }
  }

  return std::shared_ptr<Bookmark>();
}

std::vector<std::shared_ptr<Bookmark>> BookmarkController::getAllBookmarks() const {
  LOG_INFO("Retrieving all bookmarks");

  std::vector<std::shared_ptr<Bookmark>> bookmarks;

  for(const std::shared_ptr<NodeBookmark>& nodeBookmark : getAllNodeBookmarks()) {
    bookmarks.push_back(nodeBookmark);
  }
  for(const std::shared_ptr<EdgeBookmark>& edgeBookmark : getAllEdgeBookmarks()) {
    bookmarks.push_back(edgeBookmark);
  }

  return bookmarks;
}

std::vector<std::shared_ptr<NodeBookmark>> BookmarkController::getAllNodeBookmarks() const {
  const auto nodeBookmarks = mBookmarkCache.getAllNodeBookmarks();
  return nodeBookmarks | ranges::cpp20::views::transform([](const NodeBookmark& nodeBookmark) {
           return std::make_shared<NodeBookmark>(nodeBookmark);
         }) |
      ranges::to<std::vector<std::shared_ptr<NodeBookmark>>>();
}

std::vector<std::shared_ptr<EdgeBookmark>> BookmarkController::getAllEdgeBookmarks() const {
  const auto edgeBookmarks = mBookmarkCache.getAllEdgeBookmarks();
  return edgeBookmarks | ranges::cpp20::views::transform([](const EdgeBookmark& edgeBookmark) {
           return std::make_shared<EdgeBookmark>(edgeBookmark);
         }) |
      ranges::to<std::vector<std::shared_ptr<EdgeBookmark>>>();
}

std::vector<std::shared_ptr<Bookmark>> BookmarkController::getBookmarks(Bookmark::Filter filter, Bookmark::Order order) const {
  LOG_INFO(
      fmt::format("Retrieving bookmarks with filter \"{}\" and order \"{}\"", static_cast<int>(filter), static_cast<int>(order)));

  std::vector<std::shared_ptr<Bookmark>> bookmarks = getAllBookmarks();
  bookmarks = getFilteredBookmarks(bookmarks, filter);
  bookmarks = getOrderedBookmarks(bookmarks, order);
  return bookmarks;
}

std::vector<std::wstring> BookmarkController::getActiveNodeDisplayNames() const {
  const auto activeNodeIds = mActiveNodeIds[TabId::currentTab()];
  return activeNodeIds | ranges::cpp20::views::transform([this](Id nodeId) { return getNodeDisplayName(nodeId); }) |
      ranges::to<std::vector>();
}

std::vector<std::wstring> BookmarkController::getActiveEdgeDisplayNames() const {
  std::vector<std::wstring> activeEdgeDisplayNames;
  for(Id activeEdgeId : mActiveEdgeIds[TabId::currentTab()]) {
    const StorageEdge activeEdge = mStorageAccess->getEdgeById(activeEdgeId);
    const std::wstring sourceDisplayName = getNodeDisplayName(activeEdge.sourceNodeId);
    const std::wstring targetDisplayName = getNodeDisplayName(activeEdge.targetNodeId);
    activeEdgeDisplayNames.push_back(sourceDisplayName + sEdgeSeparatorToken + targetDisplayName);
  }
  return activeEdgeDisplayNames;
}

std::wstring BookmarkController::getNodeDisplayName(const Id nodeId) const {
  NodeType type = mStorageAccess->getNodeTypeForNodeWithId(nodeId);
  NameHierarchy nameHierarchy = mStorageAccess->getNameHierarchyForNodeId(nodeId);

  if(type.isFile()) {
    return FilePath(nameHierarchy.getQualifiedName()).fileName();
  }

  return nameHierarchy.getQualifiedName();
}

std::vector<std::shared_ptr<Bookmark>> BookmarkController::getFilteredBookmarks(
    const std::vector<std::shared_ptr<Bookmark>>& bookmarks, Bookmark::Filter filter) const {
  std::vector<std::shared_ptr<Bookmark>> result;

  if(filter == Bookmark::Filter::All) {
    return bookmarks;
  } else if(filter == Bookmark::Filter::Nodes) {
    for(const std::shared_ptr<Bookmark>& bookmark : bookmarks) {
      if(std::dynamic_pointer_cast<NodeBookmark>(bookmark)) {
        result.push_back(bookmark);
      }
    }
  } else if(filter == Bookmark::Filter::Edges) {
    for(const std::shared_ptr<Bookmark>& bookmark : bookmarks) {
      if(std::dynamic_pointer_cast<EdgeBookmark>(bookmark)) {
        result.push_back(bookmark);
      }
    }
  }

  return result;
}

std::vector<std::shared_ptr<Bookmark>> BookmarkController::getOrderedBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks,
                                                                               Bookmark::Order order) const {
  std::vector<std::shared_ptr<Bookmark>> result = bookmarks;

  if(order == Bookmark::Order::DateAscending) {
    return getDateOrderedBookmarks(result, true);
  } else if(order == Bookmark::Order::DateDescending) {
    return getDateOrderedBookmarks(result, false);
  } else if(order == Bookmark::Order::NameAscending) {
    return getNameOrderedBookmarks(result, true);
  } else if(order == Bookmark::Order::NameDescending) {
    return getNameOrderedBookmarks(result, false);
  }

  return result;
}

std::vector<std::shared_ptr<Bookmark>> BookmarkController::getDateOrderedBookmarks(
    const std::vector<std::shared_ptr<Bookmark>>& bookmarks, const bool ascending) const {
  std::vector<std::shared_ptr<Bookmark>> result = bookmarks;

  std::sort(result.begin(), result.end(), BookmarkController::bookmarkDateCompare);

  if(ascending == false) {
    std::reverse(result.begin(), result.end());
  }

  return result;
}

std::vector<std::shared_ptr<Bookmark>> BookmarkController::getNameOrderedBookmarks(
    const std::vector<std::shared_ptr<Bookmark>>& bookmarks, const bool ascending) const {
  std::vector<std::shared_ptr<Bookmark>> result = bookmarks;

  std::sort(result.begin(), result.end(), BookmarkController::bookmarkNameCompare);

  if(ascending == false) {
    std::reverse(result.begin(), result.end());
  }

  return result;
}

void BookmarkController::cleanBookmarkCategories() {
  mBookmarkCache.clear();

  const std::vector<std::shared_ptr<Bookmark>>& bookmarks = getAllBookmarks();

  for(const BookmarkCategory& category : getAllBookmarkCategories()) {
    const bool used = ranges::any_of(
        bookmarks, [category](const auto& bookmark) { return bookmark->getCategory().getName() == category.getName(); });

    if(!used) {
      mStorageAccess->removeBookmarkCategory(category.getId());
    }
  }
}

bool BookmarkController::bookmarkDateCompare(const std::shared_ptr<Bookmark>& a, const std::shared_ptr<Bookmark>& b) {
  return a->getTimeStamp() < b->getTimeStamp();
}

bool BookmarkController::bookmarkNameCompare(const std::shared_ptr<Bookmark>& a, const std::shared_ptr<Bookmark>& b) {
  std::wstring aName = a->getName();
  std::wstring bName = b->getName();

  aName = utility::toLowerCase(aName);
  bName = utility::toLowerCase(bName);

  unsigned int i = 0;
  while(i < aName.length() && i < bName.length()) {
    if(std::towlower(static_cast<wint_t>(aName[i])) < std::towlower(static_cast<wint_t>(bName[i]))) {
      return true;
    } else if(std::towlower(static_cast<wint_t>(aName[i])) > std::towlower(static_cast<wint_t>(bName[i]))) {
      return false;
    }

    ++i;
  }

  return aName.length() < bName.length();
}

void BookmarkController::update() {
  BookmarkView* view = getView<BookmarkView>();
  if(view == nullptr) {
    LOG_WARNING("Failed to get view");
    return;
  }

  if(view->bookmarkBrowserIsVisible()) {
    view->displayBookmarks(getBookmarks(mFilter, mOrder));
  }

  std::vector<std::shared_ptr<Bookmark>> bookmarks = getBookmarks(Bookmark::Filter::All, Bookmark::Order::DateDescending);

  constexpr size_t maxBookmarkMenuCount = 20;
  if(bookmarks.size() > maxBookmarkMenuCount) {
    bookmarks.resize(maxBookmarkMenuCount);
  }

  MessageBookmarkUpdate(std::move(bookmarks)).dispatchImmediately();
}
