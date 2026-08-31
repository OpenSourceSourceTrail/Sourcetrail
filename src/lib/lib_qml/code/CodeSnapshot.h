#pragma once
#include <QList>
#include <QString>

#include "GlobalId.hpp"

namespace code {

/**
 * One source location, already resolved to character offsets inside its snippet's text.
 *
 * The offsets are computed once, where the snippet text is still at hand, because both consumers
 * need them: the decoration pass drives a QTextCursor with them, and a click is hit-tested by
 * asking the TextArea for a character position and finding the span that contains it. Recomputing
 * them from line and column at either site would mean re-deriving the line index twice.
 */
struct LocationSpan final {
  int startOffset = 0;
  int endOffset = 0;    ///< One past the last character, as QTextCursor expects.

  int startLine = 0;
  int endLine = 0;

  int type = 0;    ///< LocationType.
  qulonglong locationId = 0;
  QList<qulonglong> tokenIds;

  bool isActive = false;
  bool isFocused = false;
  bool isUnsolved = false;
};

/** One contiguous run of lines from a file, as SnippetMerger produced it. */
struct SnippetItem final {
  QString title;
  QString footer;
  QString code;

  int startLine = 1;
  int endLine = 1;

  qulonglong titleId = 0;
  qulonglong footerId = 0;

  QList<LocationSpan> locations;
  bool isOverview = false;
};

/** A file and the snippets shown for it. */
struct FileItem final {
  QString filePath;
  QString fileName;
  QString language;    ///< Selects the highlighting rule set; matches a `*.rules` basename.

  int referenceCount = 0;

  bool isMinimized = false;
  bool isDeclaration = false;
  bool isDefinition = false;
  bool isComplete = true;

  QList<SnippetItem> snippets;
};

/** Everything the code panel draws, handed over the thread hop in one piece. */
struct CodeSnapshot final {
  QList<FileItem> files;

  int referenceCount = 0;
  int referenceIndex = 0;
  int localReferenceCount = 0;
  int localReferenceIndex = 0;

  bool listMode = true;
  bool showsErrors = false;
};

}    // namespace code
