#include "search/QmlSearchView.h"

#include <utility>

#include "data/search/SearchMatch.h"
#include "utilityString.h"

namespace search {

namespace {

QString toQt(const std::wstring& text) {
  return QString::fromStdString(utility::encodeToUtf8(text));
}

QList<MatchItem> toItems(const std::vector<SearchMatch>& matches) {
  QList<MatchItem> items;
  items.reserve(static_cast<qsizetype>(matches.size()));
  for(const SearchMatch& match : matches) {
    MatchItem item;
    item.name = toQt(match.name);
    item.subtext = toQt(match.subtext);
    item.typeName = toQt(match.getSearchTypeName());
    item.nodeType = static_cast<int>(match.nodeType.getKind());
    item.searchType = static_cast<int>(match.searchType);
    item.hasChildren = match.hasChildren;
    for(const size_t index : match.indices) {
      item.indices.append(static_cast<int>(index));
    }
    for(const Id tokenId : match.tokenIds) {
      item.tokenIds.append(static_cast<qulonglong>(tokenId));
    }
    items.append(std::move(item));
  }
  return items;
}

}    // namespace

QmlSearchView::QmlSearchView(MatchHandler onMatches, MatchHandler onAutocompletions, FocusHandler onFocus)
    : SearchView(nullptr)
    , mOnMatches(std::move(onMatches))
    , mOnAutocompletions(std::move(onAutocompletions))
    , mOnFocus(std::move(onFocus)) {}

QmlSearchView::~QmlSearchView() = default;

void QmlSearchView::setQueryFromGui(const std::wstring& query) {
  const std::lock_guard<std::mutex> lock(mQueryMutex);
  mQuery = query;
}

void QmlSearchView::refreshView() {}

std::wstring QmlSearchView::getQuery() const {
  const std::lock_guard<std::mutex> lock(mQueryMutex);
  return mQuery;
}

void QmlSearchView::setMatches(const std::vector<SearchMatch>& matches) {
  if(mOnMatches) {
    mOnMatches(toItems(matches));
  }
}

void QmlSearchView::setAutocompletionList(const std::vector<SearchMatch>& autocompletionList) {
  if(mOnAutocompletions) {
    mOnAutocompletions(toItems(autocompletionList));
  }
}

void QmlSearchView::setFocus() {
  if(mOnFocus) {
    mOnFocus(false);
  }
}

void QmlSearchView::findFulltext() {
  if(mOnFocus) {
    mOnFocus(true);
  }
}

}    // namespace search
