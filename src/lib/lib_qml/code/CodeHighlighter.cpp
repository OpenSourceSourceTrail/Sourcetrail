#include "code/CodeHighlighter.h"

#include <array>
#include <utility>

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTextDocument>

#include "app/paths/ResourcePaths.h"
#include "FileSystem.h"
#include "logging.h"
#include "settings/ColorScheme.h"
#include "TextAccess.h"

namespace code {

namespace {

/** The `syntax/*` keys the colour scheme defines, which are also the rule files' `type` values. */
constexpr std::array<const char*, 8> kSyntaxTypes = {
    "comment", "directive", "function", "keyword", "number", "quotation", "text", "type"};

QTextCharFormat formatFor(const QString& type, const std::map<QString, QTextCharFormat>& formats) {
  const auto found = formats.find(type);
  return found != formats.end() ? found->second : QTextCharFormat{};
}

}    // namespace

std::map<std::wstring, CodeHighlighter::RuleSet> CodeHighlighter::sRuleSets;

void CodeHighlighter::loadRules() {
  sRuleSets.clear();

  // Foreground only -- see the class comment. A background here would fight the location layer.
  std::map<QString, QTextCharFormat> formats;
  const std::shared_ptr<ColorScheme> scheme = ColorScheme::getInstance();
  for(const char* type : kSyntaxTypes) {
    QTextCharFormat format;
    format.setForeground(QColor(QString::fromStdString(scheme->getSyntaxColor(type))));
    formats.emplace(QString::fromLatin1(type), std::move(format));
  }

  const FilePath rulesDirectory = ResourcePaths::getSyntaxHighlightingRulesDirectoryPath();
  for(const FilePath& path : FileSystem::getFilePathsFromDirectory(rulesDirectory, {L".rules"})) {
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(
        QString::fromStdString(TextAccess::createFromFile(path)->getText()).toUtf8(), &error);
    if(!document.isArray()) {
      LOG_ERROR(fmt::format("Highlighting rules in \"{}\" are not a JSON array: offset {} - {}",
                            path.str(),
                            error.offset,
                            error.errorString().toStdString()));
      continue;
    }

    RuleSet ruleSet;
    for(const QJsonValue value : document.array()) {
      const QJsonObject object = value.toObject();
      const QTextCharFormat format = formatFor(object.value(QStringLiteral("type")).toString(), formats);
      const bool priority = object.value(QStringLiteral("priority")).toBool();

      for(const QJsonValue pattern : object.value(QStringLiteral("patterns")).toArray()) {
        QRegularExpression expression{pattern.toString()};
        if(!expression.isValid()) {
          LOG_ERROR(fmt::format(
              "Ignoring invalid highlighting pattern \"{}\" in \"{}\".", pattern.toString().toStdString(), path.str()));
          continue;
        }
        expression.optimize();
        ruleSet.rules.push_back(Rule{std::move(expression), format, priority});
      }

      const QJsonObject range = object.value(QStringLiteral("range")).toObject();
      if(!range.isEmpty()) {
        QRegularExpression start{range.value(QStringLiteral("start")).toString()};
        QRegularExpression end{range.value(QStringLiteral("end")).toString()};
        if(start.isValid() && end.isValid()) {
          start.optimize();
          end.optimize();
          ruleSet.ranges.push_back(Range{std::move(start), std::move(end), format});
        }
      }
    }

    sRuleSets.emplace(path.withoutExtension().fileName(), std::move(ruleSet));
  }
}

void CodeHighlighter::clearRules() {
  sRuleSets.clear();
}

bool CodeHighlighter::supports(const std::wstring& language) {
  return sRuleSets.find(language) != sRuleSets.end();
}

CodeHighlighter::CodeHighlighter(QTextDocument* document, const std::wstring& language) : QSyntaxHighlighter(document) {
  const auto found = sRuleSets.find(language);
  mRuleSet = found != sRuleSets.end() ? &found->second : nullptr;
}

CodeHighlighter::~CodeHighlighter() = default;

int CodeHighlighter::continueOpenRange(const QString& text) {
  // A block's state is the index of the range still open when it ended, or -1 for none. Qt hands us
  // the previous block's state, which is how a comment survives across lines without us tracking it.
  const int openRange = previousBlockState();
  if(openRange < 0 || openRange >= static_cast<int>(mRuleSet->ranges.size())) {
    return 0;
  }

  const Range& range = mRuleSet->ranges.at(static_cast<size_t>(openRange));
  const QRegularExpressionMatch match = range.end.match(text);
  if(!match.hasMatch()) {
    setFormat(0, text.length(), range.format);
    setCurrentBlockState(openRange);
    return text.length();
  }

  const int end = match.capturedEnd();
  setFormat(0, end, range.format);
  return end;
}

void CodeHighlighter::highlightBlock(const QString& text) {
  if(mRuleSet == nullptr) {
    return;
  }
  setCurrentBlockState(-1);

  const int resume = continueOpenRange(text);
  if(currentBlockState() >= 0) {
    return;    // the whole block is inside a still-open range
  }

  // Left to right, letting whichever construct starts first consume the text. That ordering is what
  // keeps a "/*" inside a string literal from opening a comment: the string matches earlier and the
  // scan resumes past it. Priority rules (strings, comments) and range starts compete here; the rest
  // are applied afterwards, only where nothing has claimed the text.
  QList<QPair<int, int>> claimed;
  int position = resume;
  while(position < text.length()) {
    int bestStart = text.length();
    int bestEnd = -1;
    const Range* bestRange = nullptr;
    const Rule* bestRule = nullptr;

    for(const Rule& rule : mRuleSet->rules) {
      if(!rule.priority) {
        continue;
      }
      const QRegularExpressionMatch match = rule.pattern.match(text, position);
      if(match.hasMatch() && match.capturedStart() < bestStart) {
        bestStart = match.capturedStart();
        bestEnd = match.capturedEnd();
        bestRule = &rule;
        bestRange = nullptr;
      }
    }
    for(const Range& range : mRuleSet->ranges) {
      const QRegularExpressionMatch match = range.start.match(text, position);
      if(match.hasMatch() && match.capturedStart() < bestStart) {
        bestStart = match.capturedStart();
        bestEnd = match.capturedEnd();
        bestRange = &range;
        bestRule = nullptr;
      }
    }

    if(bestRange == nullptr && bestRule == nullptr) {
      break;
    }

    if(bestRange != nullptr) {
      const QRegularExpressionMatch closing = bestRange->end.match(text, bestEnd);
      const int end = closing.hasMatch() ? closing.capturedEnd() : text.length();
      setFormat(bestStart, end - bestStart, bestRange->format);
      claimed.append({bestStart, end});
      if(!closing.hasMatch()) {
        const auto index = static_cast<int>(bestRange - mRuleSet->ranges.data());
        setCurrentBlockState(index);
        return;
      }
      position = end;
    } else {
      setFormat(bestStart, bestEnd - bestStart, bestRule->format);
      claimed.append({bestStart, bestEnd});
      // An empty match would spin here; step over it.
      position = bestEnd > bestStart ? bestEnd : bestStart + 1;
    }
  }

  const auto isClaimed = [&claimed](int start, int end) {
    for(const auto& [claimedStart, claimedEnd] : claimed) {
      if(start < claimedEnd && end > claimedStart) {
        return true;
      }
    }
    return false;
  };

  for(const Rule& rule : mRuleSet->rules) {
    if(rule.priority) {
      continue;
    }
    QRegularExpressionMatchIterator matches = rule.pattern.globalMatch(text, resume);
    while(matches.hasNext()) {
      const QRegularExpressionMatch match = matches.next();
      if(!isClaimed(match.capturedStart(), match.capturedEnd())) {
        setFormat(match.capturedStart(), match.capturedLength(), rule.format);
      }
    }
  }
}

}    // namespace code
