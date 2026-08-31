#pragma once
#include <map>
#include <string>
#include <vector>

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QTextDocument;

namespace code {

/**
 * Syntax colouring for one code document.
 *
 * The rules and the colours are the ones the widget GUI used -- `data/syntax_highlighting_rules/
 * <language>.rules` and ColorScheme's `syntax/*` keys -- but the traversal is Qt's. QSyntaxHighlighter
 * already walks blocks, tracks a per-block state across edits and re-runs only what changed, which is
 * what the old QtHighlighter hand-rolled in 459 lines of range bookkeeping.
 *
 * **This class sets foreground only, and that is load-bearing.** Source-location decoration is applied
 * to the same document as a *background* through a QTextCursor. Qt keeps the two apart: a highlighter's
 * formats live on the block's QTextLayout, cursor-applied formats live in the document's character
 * formats, and they are merged at paint time. Because the two layers set disjoint properties they
 * compose instead of overwriting each other, so neither has to know about the other. Set a background
 * here and it would win over the location layer wherever the two overlap.
 */
class CodeHighlighter final : public QSyntaxHighlighter {
  Q_OBJECT

public:
  /** Reads every `*.rules` file and resolves the syntax colours. Call once the ColorScheme is loaded. */
  static void loadRules();
  static void clearRules();

  /** True when a rule set was found for @p language, i.e. colouring it will do something. */
  static bool supports(const std::wstring& language);

  CodeHighlighter(QTextDocument* document, const std::wstring& language);
  ~CodeHighlighter() override;

protected:
  void highlightBlock(const QString& text) override;

private:
  struct Rule final {
    QRegularExpression pattern;
    QTextCharFormat format;
    bool priority = false;
  };

  /** A block-comment-style rule, spanning any number of blocks. */
  struct Range final {
    QRegularExpression start;
    QRegularExpression end;
    QTextCharFormat format;
  };

  struct RuleSet final {
    std::vector<Rule> rules;
    std::vector<Range> ranges;
  };

  /** Colours @p text from the range that was still open when the previous block ended.
   *  @return where that range stopped, i.e. where single-line rules may start matching. */
  int continueOpenRange(const QString& text);

  static std::map<std::wstring, RuleSet> sRuleSets;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  const RuleSet* mRuleSet = nullptr;
};

}    // namespace code
