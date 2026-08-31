#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "component/view/SearchView.h"
#include "search/SearchSnapshot.h"

namespace search {

/**
 * The SearchView the controller reads the current query from and pushes results into.
 *
 * Unlike every other view here, this one is not write-only: SearchController::handleMessage
 * (MessageSearchAutocomplete) calls getQuery() **on the message-bus thread** and drops the response
 * if it no longer matches what the user has typed. That is the debounce that keeps stale
 * completions off the screen, so the query has to be readable from that thread -- hence the mutex.
 * It guards a single string and is never held across a callback.
 */
class QmlSearchView final : public SearchView {
public:
  using MatchHandler = std::function<void(QList<MatchItem>)>;
  using FocusHandler = std::function<void(bool fulltext)>;

  QmlSearchView(MatchHandler onMatches, MatchHandler onAutocompletions, FocusHandler onFocus);
  ~QmlSearchView() override;

  /** Called from the GUI thread when the user types. */
  void setQueryFromGui(const std::wstring& query);

  void refreshView() override;

  [[nodiscard]] std::wstring getQuery() const override;
  void setMatches(const std::vector<SearchMatch>& matches) override;
  void setAutocompletionList(const std::vector<SearchMatch>& autocompletionList) override;
  void setFocus() override;
  void findFulltext() override;

private:
  MatchHandler mOnMatches;
  MatchHandler mOnAutocompletions;
  FocusHandler mOnFocus;

  mutable std::mutex mQueryMutex;
  std::wstring mQuery;
};

}    // namespace search
