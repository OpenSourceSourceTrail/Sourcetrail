#pragma once
#include <memory>

#include <QObject>
#include <QString>

#include <qqmlintegration.h>

#include "search/ActionModel.h"
#include "search/MatchModel.h"

class Component;
class QJSEngine;
class QQmlEngine;
class SearchController;
class StorageAccess;

namespace search {

class QmlSearchView;

/**
 * Symbol search and the command palette.
 *
 * SearchController answers autocompletion from the index and turns a chosen match into an
 * activation; this exposes both to QML as list models, and adds the fixed command list the palette
 * shows beneath the symbol matches.
 *
 * Keystrokes are not debounced here. MessageFilterSearchAutocomplete already coalesces them on the
 * bus, and SearchController drops any response whose query no longer matches what is typed -- a
 * second timer in the view-model would only add latency.
 */
class SearchViewModel final : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  /** What the user has typed. Writing it asks the index for completions. */
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

  Q_PROPERTY(QAbstractItemModel* completions READ completions CONSTANT)
  Q_PROPERTY(QAbstractItemModel* matches READ matches CONSTANT)
  Q_PROPERTY(QAbstractItemModel* actions READ actions CONSTANT)

  /** True while the palette is open; QML binds its visibility to this. */
  Q_PROPERTY(bool paletteOpen READ paletteOpen WRITE setPaletteOpen NOTIFY paletteOpenChanged)

public:
  explicit SearchViewModel(QObject* parent);
  ~SearchViewModel() override;

  static SearchViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

  /** Called by AppShell::setup(), once the message queue exists. */
  void attach(StorageAccess* storageAccess);

  [[nodiscard]] QString query() const;
  void setQuery(const QString& query);

  [[nodiscard]] QAbstractItemModel* completions();
  [[nodiscard]] QAbstractItemModel* matches();
  [[nodiscard]] QAbstractItemModel* actions();

  [[nodiscard]] bool paletteOpen() const;
  void setPaletteOpen(bool open);

  /** @name What QML does. All dispatch a message or run an action and return. @{ */
  Q_INVOKABLE void activateCompletion(int row);
  Q_INVOKABLE void runAction(int row);
  Q_INVOKABLE void searchFulltext(const QString& text);
  Q_INVOKABLE void clear();
  /** @} */

Q_SIGNALS:
  void queryChanged();
  void completionsChanged();
  void matchesChanged();
  void paletteOpenChanged();
  /** The bus asked for the search field; QML decides how to give it focus. */
  void focusRequested(bool fulltext);

private:
  void buildActions();

  static SearchViewModel* sInstance;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  MatchModel mCompletions;
  MatchModel mMatches;
  ActionModel mActions;

  std::shared_ptr<QmlSearchView> mView;
  std::shared_ptr<SearchController> mController;
  std::unique_ptr<Component> mComponent;

  QString mQuery;
  bool mPaletteOpen = false;
};

}    // namespace search
