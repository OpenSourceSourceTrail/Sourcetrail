#include "code/CodeViewModel.h"

#include <utility>

#include <QColor>
#include <QJSEngine>
#include <QQmlEngine>
#include <QQuickTextDocument>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

#include "code/CodeHighlighter.h"
#include "code/QmlCodeView.h"
#include "component/Component.h"
#include "component/controller/CodeController.h"
#include "GuiThread.h"
#include "logging.h"
#include "MessageQueue.h"
#include "settings/ColorScheme.h"
#include "type/code/MessageActivateSourceLocations.h"
#include "type/code/MessageChangeFileView.h"
#include "type/code/MessageCodeReference.h"
#include "utilityString.h"

namespace code {

namespace {

/** The colour-scheme name for a location type, or empty where the type is not decorated. */
std::string annotationTypeFor(int locationType) {
  switch(static_cast<LocationType>(locationType)) {
  case LOCATION_TOKEN:
  case LOCATION_UNSOLVED:
    return "token";
  case LOCATION_SCOPE:
    return "scope";
  case LOCATION_LOCAL_SYMBOL:
    return "local_symbol";
  case LOCATION_ERROR:
    return "error";
  case LOCATION_FULLTEXT_SEARCH:
    return "fulltext_search";
  case LOCATION_SCREEN_SEARCH:
    return "screen_search";
  default:
    return {};
  }
}

}    // namespace

CodeViewModel* CodeViewModel::sInstance = nullptr;

CodeViewModel::CodeViewModel(QObject* parent) : QObject(parent) {
  sInstance = this;
}

CodeViewModel::~CodeViewModel() {
  sInstance = nullptr;
}

CodeViewModel* CodeViewModel::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
  // AppShell owns the view-model for the whole run; the QML engine must not delete it.
  QJSEngine::setObjectOwnership(sInstance, QJSEngine::CppOwnership);
  return sInstance;
}

void CodeViewModel::attach(StorageAccess* storageAccess) {
  if(mComponent) {
    return;
  }
  // CodeController registers with IMessageQueue in its constructor, so the queue has to exist first.
  if(!IMessageQueue::getInstance()) {
    LOG_ERROR("Cannot build the code component before the message queue exists.");
    return;
  }

  mView = std::make_shared<QmlCodeView>(
      [this](CodeSnapshot snapshot) {
        // Called on the message-bus thread. Hand the whole thing over by value and return.
        qml::postToGui(this, [this, snapshot = std::move(snapshot)]() mutable { apply(std::move(snapshot)); });
      },
      [this](CodeScrollParams params, bool animated) {
        const int line = static_cast<int>(params.line);
        qml::postToGui(this, [this, line, animated]() { Q_EMIT scrollRequested(-1, line, animated); });
      });
  mController = std::make_shared<CodeController>(storageAccess);
  mComponent = std::make_unique<Component>(mView, mController);
}

void CodeViewModel::apply(CodeSnapshot snapshot) {
  const bool modeChanged = mSnapshot.listMode != snapshot.listMode;
  mSnapshot = std::move(snapshot);

  mFiles.setFiles(mSnapshot.files);
  mSnippets.setFiles(mSnapshot.files);

  Q_EMIT snapshotChanged();
  Q_EMIT referencesChanged();
  if(modeChanged) {
    Q_EMIT listModeChanged();
  }
}

QAbstractItemModel* CodeViewModel::files() {
  return &mFiles;
}

QAbstractItemModel* CodeViewModel::snippets() {
  return &mSnippets;
}

int CodeViewModel::referenceCount() const {
  return mSnapshot.referenceCount;
}

int CodeViewModel::referenceIndex() const {
  return mSnapshot.referenceIndex;
}

int CodeViewModel::localReferenceCount() const {
  return mSnapshot.localReferenceCount;
}

int CodeViewModel::localReferenceIndex() const {
  return mSnapshot.localReferenceIndex;
}

bool CodeViewModel::listMode() const {
  return mSnapshot.listMode;
}

bool CodeViewModel::empty() const {
  return mSnapshot.files.isEmpty();
}

void CodeViewModel::decorate(QQuickTextDocument* document, int snippetRow) {
  if(document == nullptr || document->textDocument() == nullptr) {
    return;
  }
  QTextDocument* textDocument = document->textDocument();

  // A recycled delegate arrives with a highlighter already attached. Re-highlighting it is right;
  // attaching a second one would double every format.
  auto* highlighter = textDocument->findChild<CodeHighlighter*>(QString{}, Qt::FindDirectChildrenOnly);
  if(highlighter == nullptr) {
    const std::wstring language = utility::decodeFromUtf8(mSnippets.languageAt(snippetRow).toStdString());
    // Parented to the document, so it dies with the delegate that owns it.
    highlighter = new CodeHighlighter(textDocument, language);
  }

  // Backgrounds only -- the highlighter owns the foreground. See CodeHighlighter's class comment.
  const std::shared_ptr<ColorScheme> scheme = ColorScheme::getInstance();
  QTextCursor cursor{textDocument};
  cursor.select(QTextCursor::Document);
  QTextCharFormat clear;
  clear.setBackground(Qt::transparent);
  cursor.setCharFormat(clear);

  for(const LocationSpan& span : mSnippets.locationsAt(snippetRow)) {
    const std::string type = annotationTypeFor(span.type);
    if(type.empty()) {
      continue;
    }
    const ColorScheme::ColorState state = span.isActive ? ColorScheme::ACTIVE :
        span.isFocused                                  ? ColorScheme::FOCUS :
                                                          ColorScheme::NORMAL;
    const QColor fill{QString::fromStdString(scheme->getCodeAnnotationTypeColor(type, "fill", state))};
    if(!fill.isValid() || fill.alpha() == 0) {
      continue;
    }

    QTextCursor span_cursor{textDocument};
    span_cursor.setPosition(span.startOffset);
    span_cursor.setPosition(span.endOffset, QTextCursor::KeepAnchor);
    QTextCharFormat format;
    format.setBackground(fill);
    span_cursor.mergeCharFormat(format);
  }

  highlighter->rehighlight();
}

void CodeViewModel::activateLocationAt(int snippetRow, int position) {
  std::vector<Id> locationIds;
  bool containsUnsolved = false;
  for(const LocationSpan& span : mSnippets.locationsAt(snippetRow)) {
    if(position >= span.startOffset && position < span.endOffset && !span.tokenIds.isEmpty()) {
      locationIds.push_back(static_cast<Id>(span.locationId));
      containsUnsolved = containsUnsolved || span.isUnsolved;
    }
  }
  if(locationIds.empty()) {
    return;
  }
  MessageActivateSourceLocations{locationIds, containsUnsolved}.dispatch();
}

void CodeViewModel::nextReference() {
  MessageCodeReference{MessageCodeReference::REFERENCE_NEXT, false}.dispatch();
}

void CodeViewModel::previousReference() {
  MessageCodeReference{MessageCodeReference::REFERENCE_PREVIOUS, false}.dispatch();
}

void CodeViewModel::setListMode(bool listMode) {
  if(mSnapshot.listMode == listMode) {
    return;
  }
  const FilePath filePath = mSnapshot.files.isEmpty() ? FilePath{} : FilePath{mSnapshot.files.first().filePath.toStdString()};
  MessageChangeFileView{filePath,
                        listMode ? MessageChangeFileView::FILE_SNIPPETS : MessageChangeFileView::FILE_MAXIMIZED,
                        listMode ? MessageChangeFileView::VIEW_LIST : MessageChangeFileView::VIEW_SINGLE,
                        CodeScrollParams{},
                        true}
      .dispatch();
}

void CodeViewModel::showFile(const QString& filePath, bool maximized) {
  const FilePath path{filePath.toStdString()};
  MessageChangeFileView{path,
                        maximized ? MessageChangeFileView::FILE_MAXIMIZED : MessageChangeFileView::FILE_SNIPPETS,
                        MessageChangeFileView::VIEW_CURRENT,
                        CodeScrollParams::toFile(path, CodeScrollParams::Target::VISIBLE)}
      .dispatch();
}

}    // namespace code
