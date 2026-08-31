#include "search/SearchViewModel.h"

#include <utility>

#include <QJSEngine>
#include <QQmlEngine>

#include "component/Component.h"
#include "component/controller/SearchController.h"
#include "data/NodeTypeSet.h"
#include "data/search/SearchMatch.h"
#include "GuiThread.h"
#include "logging.h"
#include "MessageQueue.h"
#include "search/QmlSearchView.h"
#include "type/activation/MessageActivateOverview.h"
#include "type/MessageRefresh.h"
#include "type/search/MessageSearch.h"
#include "type/search/MessageSearchAutocomplete.h"
#include "utilityString.h"

namespace search {

namespace {

std::wstring toStd(const QString& text) {
  return utility::decodeFromUtf8(text.toStdString());
}

}    // namespace

SearchViewModel* SearchViewModel::sInstance = nullptr;

SearchViewModel::SearchViewModel(QObject* parent) : QObject(parent) {
  sInstance = this;
  buildActions();
}

SearchViewModel::~SearchViewModel() {
  sInstance = nullptr;
}

SearchViewModel* SearchViewModel::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
  // AppShell owns the view-model for the whole run; the QML engine must not delete it.
  QJSEngine::setObjectOwnership(sInstance, QJSEngine::CppOwnership);
  return sInstance;
}

void SearchViewModel::attach(StorageAccess* storageAccess) {
  if(mComponent) {
    return;
  }
  // SearchController registers with IMessageQueue in its constructor, so the queue has to exist
  // first. AppShell::setup() is called from inside Application::createInstance, after it is set.
  if(!IMessageQueue::getInstance()) {
    LOG_ERROR("Cannot build the search component before the message queue exists.");
    return;
  }

  mView = std::make_shared<QmlSearchView>(
      [this](QList<MatchItem> items) {
        // Called on the message-bus thread. Hand the whole thing over by value and return.
        qml::postToGui(this, [this, items = std::move(items)]() mutable {
          mMatches.setItems(std::move(items));
          Q_EMIT matchesChanged();
        });
      },
      [this](QList<MatchItem> items) {
        qml::postToGui(this, [this, items = std::move(items)]() mutable {
          mCompletions.setItems(std::move(items));
          Q_EMIT completionsChanged();
        });
      },
      [this](bool fulltext) {
        qml::postToGui(this, [this, fulltext]() {
          setPaletteOpen(true);
          Q_EMIT focusRequested(fulltext);
        });
      });
  mController = std::make_shared<SearchController>(storageAccess);
  mComponent = std::make_unique<Component>(mView, mController);
}

QString SearchViewModel::query() const {
  return mQuery;
}

void SearchViewModel::setQuery(const QString& query) {
  if(mQuery == query) {
    return;
  }
  mQuery = query;
  Q_EMIT queryChanged();

  if(!mView) {
    return;
  }
  // The controller reads this back on the bus thread to discard stale completions, so it has to be
  // stored before the request goes out.
  mView->setQueryFromGui(toStd(mQuery));
  MessageSearchAutocomplete{toStd(mQuery), NodeTypeSet::all()}.dispatch();
}

QAbstractItemModel* SearchViewModel::completions() {
  return &mCompletions;
}

QAbstractItemModel* SearchViewModel::matches() {
  return &mMatches;
}

QAbstractItemModel* SearchViewModel::actions() {
  return &mActions;
}

bool SearchViewModel::paletteOpen() const {
  return mPaletteOpen;
}

void SearchViewModel::setPaletteOpen(bool open) {
  if(mPaletteOpen == open) {
    return;
  }
  mPaletteOpen = open;
  Q_EMIT paletteOpenChanged();
}

void SearchViewModel::activateCompletion(int row) {
  const QList<MatchItem>& items = mCompletions.items();
  if(row < 0 || row >= items.size()) {
    return;
  }

  // Rebuild the SearchMatch the controller expects. Only the fields activation reads are needed:
  // the ids identify the symbol, the name and type are what the history and the chips display.
  const MatchItem& item = items.at(row);
  SearchMatch match;
  match.name = toStd(item.name);
  match.text = match.name;
  match.subtext = toStd(item.subtext);
  match.searchType = static_cast<SearchMatch::SearchType>(item.searchType);
  match.nodeType = NodeType{static_cast<NodeKind>(item.nodeType)};
  for(const qulonglong tokenId : item.tokenIds) {
    match.tokenIds.push_back(static_cast<Id>(tokenId));
  }

  setPaletteOpen(false);
  MessageSearch{{match}, NodeTypeSet::all()}.dispatch();
}

void SearchViewModel::runAction(int row) {
  setPaletteOpen(false);
  mActions.run(row);
}

void SearchViewModel::searchFulltext(const QString& text) {
  if(text.isEmpty()) {
    return;
  }
  SearchMatch match{std::wstring{SearchMatch::FULLTEXT_SEARCH_CHARACTER} + toStd(text)};
  match.searchType = SearchMatch::SEARCH_FULLTEXT;

  setPaletteOpen(false);
  MessageSearch{{match}, NodeTypeSet::all()}.dispatch();
}

void SearchViewModel::clear() {
  setQuery(QString{});
  mCompletions.setItems({});
  Q_EMIT completionsChanged();
}

void SearchViewModel::buildActions() {
  QList<ActionItem> actions;
  actions.append(
      {QStringLiteral("overview"), tr("Show the project overview"), QStringLiteral("◈"), QStringLiteral("Ctrl+Home"), []() {
         MessageActivateOverview{}.dispatch();
       }});
  actions.append(
      {QStringLiteral("refresh"), tr("Refresh index — updated files only"), QStringLiteral("↻"), QStringLiteral("F5"), []() {
         MessageRefresh{}.dispatch();
       }});
  actions.append({QStringLiteral("reindex"), tr("Reindex everything from scratch"), QStringLiteral("↻"), QString{}, []() {
                    MessageRefresh{}.refreshAll().dispatch();
                  }});
  mActions.setActions(std::move(actions));
}

}    // namespace search
