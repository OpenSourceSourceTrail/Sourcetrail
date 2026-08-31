#include "code/QmlCodeView.h"

#include <algorithm>
#include <utility>

#include "component/view/helper/CodeSnippetParams.h"
#include "data/location/SourceLocation.h"
#include "data/location/SourceLocationFile.h"
#include "utilityString.h"

namespace code {

namespace {

QString toQt(const std::wstring& text) {
  return QString::fromStdString(utility::encodeToUtf8(text));
}

/**
 * Character offsets of the first character of every line in @p code, indexed from @p firstLine.
 *
 * Locations arrive as line and column numbers; the document wants offsets. Building the table once
 * per snippet turns every lookup into an index, instead of counting newlines per location.
 */
std::vector<int> lineOffsets(const QString& code) {
  std::vector<int> offsets{0};
  for(int index = 0; index < code.length(); ++index) {
    if(code.at(index) == QLatin1Char('\n')) {
      offsets.push_back(index + 1);
    }
  }
  return offsets;
}

/** Clamps to the snippet: a location may start or end outside the lines actually shown. */
int offsetFor(const std::vector<int>& offsets, int firstLine, size_t line, size_t column) {
  const auto relative = static_cast<long long>(line) - firstLine;
  if(relative < 0) {
    return 0;
  }
  if(relative >= static_cast<long long>(offsets.size())) {
    return offsets.empty() ? 0 : offsets.back();
  }
  // Columns are 1-based in the index, offsets are 0-based here.
  return offsets.at(static_cast<size_t>(relative)) + static_cast<int>(column > 0 ? column - 1 : 0);
}

QList<LocationSpan> flattenLocations(
    const SourceLocationFile* locationFile, const QString& code, int firstLine, int lastLine, const CodeView::CodeParams& params) {
  QList<LocationSpan> spans;
  if(locationFile == nullptr) {
    return spans;
  }

  const std::vector<int> offsets = lineOffsets(code);
  const auto contains = [](const std::vector<Id>& ids, Id id) { return std::find(ids.begin(), ids.end(), id) != ids.end(); };

  locationFile->forEachSourceLocation([&](SourceLocation* location) {
    // Locations come in start/end pairs; take the start and reach across to its partner, so each
    // pair yields exactly one span.
    if(location == nullptr || !location->isStartLocation()) {
      return;
    }
    const SourceLocation* end = location->getOtherLocation();
    if(end == nullptr) {
      return;
    }
    // Wholly outside the lines this snippet shows.
    if(static_cast<int>(end->getLineNumber()) < firstLine || static_cast<int>(location->getLineNumber()) > lastLine) {
      return;
    }

    LocationSpan span;
    span.startLine = static_cast<int>(location->getLineNumber());
    span.endLine = static_cast<int>(end->getLineNumber());
    span.startOffset = offsetFor(offsets, firstLine, location->getLineNumber(), location->getColumnNumber());
    // The end column is inclusive in the index and exclusive for a cursor.
    span.endOffset = offsetFor(offsets, firstLine, end->getLineNumber(), end->getColumnNumber() + 1);
    span.type = static_cast<int>(location->getType());
    span.locationId = static_cast<qulonglong>(location->getLocationId());
    span.isUnsolved = location->getType() == LOCATION_UNSOLVED;

    for(const Id tokenId : location->getTokenIds()) {
      span.tokenIds.append(static_cast<qulonglong>(tokenId));
      span.isActive = span.isActive || contains(params.activeTokenIds, tokenId);
    }
    span.isActive = span.isActive || contains(params.activeLocationIds, location->getLocationId());
    span.isFocused = contains(params.currentActiveLocalLocationIds, location->getLocationId());

    if(span.endOffset > span.startOffset) {
      spans.append(std::move(span));
    }
  });

  return spans;
}

SnippetItem toSnippet(const CodeSnippetParams& snippet, const CodeView::CodeParams& params) {
  SnippetItem item;
  item.title = toQt(snippet.title);
  item.footer = toQt(snippet.footer);
  item.code = QString::fromStdString(snippet.code);
  item.startLine = static_cast<int>(snippet.startLineNumber);
  item.endLine = static_cast<int>(snippet.endLineNumber);
  item.titleId = static_cast<qulonglong>(snippet.titleId);
  item.footerId = static_cast<qulonglong>(snippet.footerId);
  item.isOverview = snippet.isOverview;
  item.locations = flattenLocations(snippet.locationFile.get(), item.code, item.startLine, item.endLine, params);
  return item;
}

FileItem toFile(const CodeFileParams& file, const CodeView::CodeParams& params) {
  FileItem item;
  if(file.locationFile) {
    item.filePath = toQt(file.locationFile->getFilePath().wstr());
    item.fileName = toQt(file.locationFile->getFilePath().fileName());
    item.language = toQt(file.locationFile->getLanguage());
    item.isComplete = file.locationFile->isComplete();
  }
  item.referenceCount = static_cast<int>(file.referenceCount);
  item.isMinimized = file.isMinimized;
  item.isDeclaration = file.isDeclaration;
  item.isDefinition = file.isDefinition;

  if(file.fileParams) {
    item.snippets.append(toSnippet(*file.fileParams, params));
  } else {
    for(const CodeSnippetParams& snippet : file.snippetParams) {
      item.snippets.append(toSnippet(snippet, params));
    }
  }
  return item;
}

}    // namespace

QmlCodeView::QmlCodeView(SnapshotHandler onSnapshot, ScrollHandler onScroll)
    : CodeView(nullptr), mOnSnapshot(std::move(onSnapshot)), mOnScroll(std::move(onScroll)) {}

QmlCodeView::~QmlCodeView() = default;

void QmlCodeView::refreshView() {}

void QmlCodeView::clear() {
  mSingleFilePaths.clear();
  mShowsErrors = false;
  if(mOnSnapshot) {
    mOnSnapshot(CodeSnapshot{});
  }
}

void QmlCodeView::showSnippets(const std::vector<CodeFileParams>& files,
                               const CodeParams& params,
                               const CodeScrollParams& scrollParams) {
  CodeSnapshot snapshot;
  snapshot.listMode = true;
  snapshot.showsErrors = !params.errorInfos.empty();
  snapshot.referenceCount = static_cast<int>(params.referenceCount);
  snapshot.referenceIndex = static_cast<int>(params.referenceIndex);
  snapshot.localReferenceCount = static_cast<int>(params.localReferenceCount);
  snapshot.localReferenceIndex = static_cast<int>(params.localReferenceIndex);

  mSingleFilePaths.clear();
  for(const CodeFileParams& file : files) {
    if(file.locationFile) {
      mSingleFilePaths.push_back(file.locationFile->getFilePath());
    }
    snapshot.files.append(toFile(file, params));
  }

  mListMode = true;
  mShowsErrors = snapshot.showsErrors;
  mParams = params;
  if(mOnSnapshot) {
    mOnSnapshot(std::move(snapshot));
  }
  scrollTo(scrollParams, false);
}

void QmlCodeView::showSingleFile(const CodeFileParams& file, const CodeParams& params, const CodeScrollParams& scrollParams) {
  CodeSnapshot snapshot;
  snapshot.listMode = false;
  snapshot.showsErrors = !params.errorInfos.empty();
  snapshot.referenceCount = static_cast<int>(params.referenceCount);
  snapshot.referenceIndex = static_cast<int>(params.referenceIndex);
  snapshot.localReferenceCount = static_cast<int>(params.localReferenceCount);
  snapshot.localReferenceIndex = static_cast<int>(params.localReferenceIndex);
  snapshot.files.append(toFile(file, params));

  mSingleFilePaths.clear();
  if(file.locationFile) {
    mSingleFilePaths.push_back(file.locationFile->getFilePath());
  }

  mListMode = false;
  mShowsErrors = snapshot.showsErrors;
  mParams = params;
  if(mOnSnapshot) {
    mOnSnapshot(std::move(snapshot));
  }
  scrollTo(scrollParams, false);
}

void QmlCodeView::updateSourceLocations(const std::vector<CodeFileParams>& files) {
  // The controller calls this when locations changed but the snippets did not. Rebuilding the whole
  // snapshot is the same work as a diff at these sizes, and keeps one path into the models.
  CodeSnapshot snapshot;
  snapshot.listMode = mListMode;
  snapshot.showsErrors = mShowsErrors;
  for(const CodeFileParams& file : files) {
    snapshot.files.append(toFile(file, mParams));
  }
  if(mOnSnapshot) {
    mOnSnapshot(std::move(snapshot));
  }
}

void QmlCodeView::scrollTo(const CodeScrollParams& params, bool animated) {
  if(mOnScroll && params.type != CodeScrollParams::Type::NONE) {
    mOnScroll(params, animated);
  }
}

bool QmlCodeView::showsErrors() const {
  return mShowsErrors;
}

void QmlCodeView::coFocusTokenIds(const std::vector<Id>& /*coFocusedTokenIds*/) {
  // Co-focus is a hover affordance; the QML delegates handle their own hover state.
}

void QmlCodeView::deCoFocusTokenIds() {}

bool QmlCodeView::isInListMode() const {
  return mListMode;
}

void QmlCodeView::setMode(bool listMode) {
  mListMode = listMode;
}

bool QmlCodeView::hasSingleFileCached(const FilePath& filePath) const {
  return std::find(mSingleFilePaths.begin(), mSingleFilePaths.end(), filePath) != mSingleFilePaths.end();
}

void QmlCodeView::setNavigationFocus(bool focus) {
  mHasNavigationFocus = focus;
}

bool QmlCodeView::hasNavigationFocus() const {
  return mHasNavigationFocus;
}

bool QmlCodeView::isVisible() const {
  // The panel is always mounted; whether it has anything to show is the snapshot's business.
  return true;
}

void QmlCodeView::findMatches(ScreenSearchSender* /*sender*/, const std::wstring& /*query*/) {
  // Screen search lands in phase 6, with the rest of the overlays.
}

void QmlCodeView::activateMatch(size_t /*matchIndex*/) {}

void QmlCodeView::deactivateMatch(size_t /*matchIndex*/) {}

void QmlCodeView::clearMatches() {}

}    // namespace code
