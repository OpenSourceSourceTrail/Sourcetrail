#include "SearchController.h"

#include "logging.h"
#include "SearchView.h"
#include "StorageAccess.h"
#include "type/tab/MessageTabState.h"

SearchController::SearchController(StorageAccess* storageAccess) : mStorageAccess(storageAccess) {}

SearchController::~SearchController() = default;

Id SearchController::getSchedulerId() const {
  return getTabId();
}

void SearchController::handleActivation(const MessageActivateBase* message) {
  if(const auto* messageActivateTokens = dynamic_cast<const MessageActivateTokens*>(message)) {
    if(!messageActivateTokens->isEdge) {
      updateMatches(message, !messageActivateTokens->keepContent());
    }
  } else if(const auto* messageActivateTrail = dynamic_cast<const MessageActivateTrail*>(message)) {
    if(messageActivateTrail->custom) {
      updateMatches(message);
    }
  } else {
    updateMatches(message);
  }
}

void SearchController::handleMessage(MessageFind* message) {
  if(message->findFulltext) {
    getView()->findFulltext();
  } else {
    getView()->setFocus();
  }
}

void SearchController::handleMessage(MessageSearchAutocomplete* message) {
  SearchView* view = getView();

  // Don't autocomplete if autocompletion request is not up-to-date anymore
  if(message->query != view->getQuery()) {
    return;
  }

  LOG_INFO(L"autocomplete string: \"{}\"", message->query);
  view->setAutocompletionList(mStorageAccess->getAutocompletionMatches(message->query, message->acceptedNodeTypes, true));
}

SearchView* SearchController::getView() const {
  return Controller::getView<SearchView>();
}

void SearchController::clear() {
  updateMatches(nullptr);
}

void SearchController::updateMatches(const MessageActivateBase* message, bool updateView) {
  std::vector<SearchMatch> matches;

  if(nullptr != message) {
    matches = message->getSearchMatches();
  }

  if(updateView) {
    getView()->setMatches(matches);
  }

  MessageTabState(getTabId(), matches).dispatch();
}
