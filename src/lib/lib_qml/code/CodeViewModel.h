#pragma once
#include <memory>

#include <QObject>
#include <QQuickTextDocument>
#include <QString>

#include <qqmlintegration.h>

#include "code/CodeModels.h"

class CodeController;
class Component;
class QJSEngine;
class QQmlEngine;
class StorageAccess;

namespace code {

class QmlCodeView;

/**
 * The code panel: the files on screen, their snippets, and reference navigation.
 *
 * Snippet text reaches QML as a model role, but source-location decoration does not. QML hands the
 * TextArea's document back through decorate(), and the QTextCursor work happens here -- attaching
 * the syntax highlighter and painting location backgrounds. Keeping it on this side is what lets the
 * two format layers stay disjoint (see CodeHighlighter), and it keeps QML free of text-document
 * plumbing it has no good way to express.
 */
class CodeViewModel final : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QAbstractItemModel* files READ files CONSTANT)
  Q_PROPERTY(QAbstractItemModel* snippets READ snippets CONSTANT)

  Q_PROPERTY(int referenceCount READ referenceCount NOTIFY referencesChanged)
  Q_PROPERTY(int referenceIndex READ referenceIndex NOTIFY referencesChanged)
  Q_PROPERTY(int localReferenceCount READ localReferenceCount NOTIFY referencesChanged)
  Q_PROPERTY(int localReferenceIndex READ localReferenceIndex NOTIFY referencesChanged)

  /** True in snippet mode, false when a single file is shown whole. */
  Q_PROPERTY(bool listMode READ listMode NOTIFY listModeChanged)
  Q_PROPERTY(bool empty READ empty NOTIFY snapshotChanged)

public:
  explicit CodeViewModel(QObject* parent);
  ~CodeViewModel() override;

  static CodeViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

  /** Called by AppShell::setup(), once the message queue exists. */
  void attach(StorageAccess* storageAccess);

  [[nodiscard]] QAbstractItemModel* files();
  [[nodiscard]] QAbstractItemModel* snippets();

  [[nodiscard]] int referenceCount() const;
  [[nodiscard]] int referenceIndex() const;
  [[nodiscard]] int localReferenceCount() const;
  [[nodiscard]] int localReferenceIndex() const;
  [[nodiscard]] bool listMode() const;
  [[nodiscard]] bool empty() const;

  /**
   * Attaches syntax highlighting and location decoration to one snippet's text document.
   *
   * Called from a delegate's Component.onCompleted with its TextArea's textDocument. Safe to call
   * again for the same document -- a recycled delegate re-decorates rather than stacking a second
   * highlighter on it.
   */
  Q_INVOKABLE void decorate(QQuickTextDocument* document, int snippetRow);

  /** @name What QML does. All dispatch a message and return. @{ */
  Q_INVOKABLE void activateLocationAt(int snippetRow, int position);
  Q_INVOKABLE void nextReference();
  Q_INVOKABLE void previousReference();
  Q_INVOKABLE void setListMode(bool listMode);
  Q_INVOKABLE void showFile(const QString& filePath, bool maximized);
  /** @} */

Q_SIGNALS:
  void snapshotChanged();
  void referencesChanged();
  void listModeChanged();
  /** The controller asked the panel to scroll; QML decides how. */
  void scrollRequested(int snippetRow, int line, bool animated);

private:
  void apply(CodeSnapshot snapshot);

  static CodeViewModel* sInstance;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  FileModel mFiles;
  SnippetModel mSnippets;

  std::shared_ptr<QmlCodeView> mView;
  std::shared_ptr<CodeController> mController;
  std::unique_ptr<Component> mComponent;

  CodeSnapshot mSnapshot;
};

}    // namespace code
