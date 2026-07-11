#include "QtBookmarkView.h"

#include <string>

#include "logging.h"
#include "QtBookmarkBrowser.h"
#include "QtBookmarkCreator.h"
#include "QtMainView.h"
#include "QtMainWindow.h"
#include "TabId.h"

namespace {
QtMainView* getMainView(ViewLayout* viewLayout) {
  auto* view = dynamic_cast<View*>(viewLayout);
  if(view == nullptr) {
    LOG_WARNING("Failed to cast \"getViewLayout()\" to View*");
    return nullptr;
  }
  auto* mainView = dynamic_cast<QtMainView*>(view->getViewLayout());
  if(mainView == nullptr) {
    LOG_WARNING("Failed to cast \"getViewLayout()\" to QtMainView*");
    return nullptr;
  }
  return mainView;
}
}    // namespace

QtBookmarkView::QtBookmarkView(ViewLayout* viewLayout) : BookmarkView(viewLayout), m_controllerProxy(this, TabId::app()) {}

QtBookmarkView::~QtBookmarkView() = default;

void QtBookmarkView::createWidgetWrapper() {}

void QtBookmarkView::refreshView() {}

void QtBookmarkView::displayBookmarkCreator(const std::vector<std::wstring>& names,
                                            const std::vector<BookmarkCategory>& categories,
                                            Id nodeId) {
  m_onQtThread([this, names, categories, nodeId]() {
    auto* mainView = getMainView(getViewLayout());
    if(mainView == nullptr) {
      return;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles the memory
    auto* bookmarkCreator = new QtBookmarkCreator(&m_controllerProxy, mainView->getMainWindow());
    bookmarkCreator->setupBookmarkCreator();

    std::wstring displayName;
    for(unsigned int i = 0; i < names.size(); i++) {
      displayName += names[i];
      if(i < names.size() - 1) {
        displayName += L"; ";
      }
    }

    bookmarkCreator->setDisplayName(displayName);
    bookmarkCreator->setBookmarkCategories(categories);
    bookmarkCreator->setNodeId(nodeId);

    bookmarkCreator->show();
    bookmarkCreator->raise();
  });
}

void QtBookmarkView::displayBookmarkEditor(std::shared_ptr<Bookmark> bookmark, const std::vector<BookmarkCategory>& categories) {
  m_onQtThread([this, bookmark, categories]() {
    auto* mainView = getMainView(getViewLayout());
    if(mainView == nullptr) {
      return;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles the memory
    auto* bookmarkCreator = new QtBookmarkCreator(&m_controllerProxy, mainView->getMainWindow(), bookmark->getId());

    bookmarkCreator->setupBookmarkCreator();
    bookmarkCreator->setDisplayName(bookmark->getName());
    bookmarkCreator->setComment(bookmark->getComment());
    bookmarkCreator->setBookmarkCategories(categories);
    bookmarkCreator->setCurrentBookmarkCategory(bookmark->getCategory());

    bookmarkCreator->show();
    bookmarkCreator->raise();
  });
}

void QtBookmarkView::displayBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks) {
  m_onQtThread([this, bookmarks]() {
    if(m_bookmarkBrowser == nullptr) {
      auto* mainView = getMainView(getViewLayout());
      if(mainView == nullptr) {
        return;
      }
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles the memory
      m_bookmarkBrowser = new QtBookmarkBrowser(&m_controllerProxy, mainView->getMainWindow());
      m_bookmarkBrowser->setupBookmarkBrowser();
    }

    m_bookmarkBrowser->setBookmarks(bookmarks);
    m_bookmarkBrowser->show();
    m_bookmarkBrowser->raise();
  });
}

bool QtBookmarkView::bookmarkBrowserIsVisible() const {
  if(m_bookmarkBrowser != nullptr) {
    return m_bookmarkBrowser->isVisible();
  } else {
    return false;
  }
}
