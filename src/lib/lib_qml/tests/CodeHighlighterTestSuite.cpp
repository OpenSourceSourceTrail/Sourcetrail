#include <memory>

#include <QColor>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>

#include <gtest/gtest.h>

#include "app/paths/AppPath.h"
#include "app/paths/ResourcePaths.h"
#include "code/CodeHighlighter.h"
#include "settings/ColorScheme.h"

using code::CodeHighlighter;

namespace {

/** The colour the highlighter paints @p position with, or an invalid colour where it painted none. */
QColor syntaxColorAt(const QTextDocument& document, int position) {
  const QTextBlock block = document.findBlock(position);
  const int offset = position - block.position();
  for(const QTextLayout::FormatRange& range : block.layout()->formats()) {
    if(offset >= range.start && offset < range.start + range.length) {
      return range.format.foreground().color();
    }
  }
  return {};
}

QColor schemeColor(const char* type) {
  return QColor{QString::fromStdString(ColorScheme::getInstance()->getSyntaxColor(type))};
}

struct CodeHighlighterFix : ::testing::Test {
  static void SetUpTestSuite() {
    // The rules live in the shared data directory, on disk -- not in the Qt resource bundle.
    AppPath::setSharedDataDirectoryPath(FilePath{SOURCETRAIL_SHARED_DATA_DIR});
    ColorScheme::getInstance()->load(ResourcePaths::getColorSchemesDirectoryPath().concatenate(L"bright.xml"));
    CodeHighlighter::loadRules();
  }

  static void TearDownTestSuite() {
    CodeHighlighter::clearRules();
  }
};

TEST_F(CodeHighlighterFix, rulesLoadForCpp) {
  EXPECT_TRUE(CodeHighlighter::supports(L"cpp"));
  EXPECT_FALSE(CodeHighlighter::supports(L"nonexistent-language"));
}

TEST_F(CodeHighlighterFix, coloursKeywordsTypesAndNumbers) {
  QTextDocument document;
  // cpp.rules files "int" under "type" and "return" under "keyword" -- they are different colours.
  document.setPlainText(QStringLiteral("int value = 1; return value;"));
  CodeHighlighter highlighter{&document, L"cpp"};
  highlighter.rehighlight();

  const QString text = document.toPlainText();
  EXPECT_EQ(syntaxColorAt(document, 0), schemeColor("type"));
  EXPECT_EQ(syntaxColorAt(document, text.indexOf(QLatin1Char('1'))), schemeColor("number"));
  EXPECT_EQ(syntaxColorAt(document, text.indexOf(QStringLiteral("return"))), schemeColor("keyword"));
}

TEST_F(CodeHighlighterFix, aKeywordInsideAStringStaysAString) {
  // The left-to-right scan exists for this: a priority rule consumes the text so later rules
  // cannot claim anything inside it.
  QTextDocument document;
  document.setPlainText(QStringLiteral("const char* s = \"int value\";"));
  CodeHighlighter highlighter{&document, L"cpp"};
  highlighter.rehighlight();

  const int insideString = document.toPlainText().indexOf(QStringLiteral("int value"));
  ASSERT_GT(insideString, 0);
  EXPECT_EQ(syntaxColorAt(document, insideString), schemeColor("quotation"));
}

TEST_F(CodeHighlighterFix, blockCommentsSpanLines) {
  QTextDocument document;
  document.setPlainText(QStringLiteral("/* comment\nint still_comment;\n*/ int code;"));
  CodeHighlighter highlighter{&document, L"cpp"};
  highlighter.rehighlight();

  const QString text = document.toPlainText();
  EXPECT_EQ(syntaxColorAt(document, text.indexOf(QStringLiteral("still_comment"))), schemeColor("comment"));
  // ... and the range closes, so the code after "*/" is highlighted normally again.
  EXPECT_EQ(syntaxColorAt(document, text.indexOf(QStringLiteral("int code"))), schemeColor("type"));
}

TEST_F(CodeHighlighterFix, aCommentOpenerInsideAStringDoesNotOpenAComment) {
  QTextDocument document;
  document.setPlainText(QStringLiteral("auto a = \"/*\";\nint after = 2;"));
  CodeHighlighter highlighter{&document, L"cpp"};
  highlighter.rehighlight();

  const QString text = document.toPlainText();
  EXPECT_EQ(syntaxColorAt(document, text.indexOf(QStringLiteral("int after"))), schemeColor("type"));
}

TEST_F(CodeHighlighterFix, locationBackgroundsSurviveRehighlighting) {
  // The whole reason CodeHighlighter sets foreground only. Source-location decoration is applied as
  // a background through a cursor; Qt keeps highlighter formats on the block layout and cursor
  // formats in the document, so the two compose rather than overwrite. If this breaks, location
  // highlighting silently disappears wherever a syntax rule matches.
  QTextDocument document;
  document.setPlainText(QStringLiteral("int value = 1;"));
  CodeHighlighter highlighter{&document, L"cpp"};

  const QColor background{QStringLiteral("#223344")};
  QTextCursor cursor{&document};
  cursor.setPosition(0);
  cursor.setPosition(3, QTextCursor::KeepAnchor);
  QTextCharFormat locationFormat;
  locationFormat.setBackground(background);
  cursor.mergeCharFormat(locationFormat);

  highlighter.rehighlight();

  QTextCursor probe{&document};
  probe.setPosition(1);
  EXPECT_EQ(probe.charFormat().background().color(), background);
  EXPECT_EQ(syntaxColorAt(document, 0), schemeColor("type"));
}

}    // namespace
