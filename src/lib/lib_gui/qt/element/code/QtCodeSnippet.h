#pragma once

#include <vector>

#include <QFrame>

#include "CodeFocusHandler.h"
#include "CodeSnippetParams.h"
#include "GlobalId.hpp"
#include "QtCodeArea.h"

class QBoxLayout;
class QPushButton;
class QtCodeFile;
class QtCodeNavigator;
class QtHoverButton;
class SourceLocationFile;

class QtCodeSnippet : public QFrame {
  Q_OBJECT

public:
  QtCodeSnippet(const CodeSnippetParams& params, QtCodeNavigator* navigator, QtCodeFile* file);
  ~QtCodeSnippet() override;

  [[nodiscard]] QtCodeFile* getFile() const;
  [[nodiscard]] QtCodeArea* getArea() const;

  [[nodiscard]] size_t getStartLineNumber() const;
  [[nodiscard]] size_t getEndLineNumber() const;

  [[nodiscard]] int lineNumberDigits() const;

  void updateSourceLocations(const CodeSnippetParams& params);

  void updateLineNumberAreaWidthForDigits(int digits);
  void updateContent();

  void setIsActiveFile(bool isActiveFile);

  [[nodiscard]] size_t getLineNumberForLocationId(Id locationId) const;
  [[nodiscard]] std::pair<size_t, size_t> getLineNumbersForLocationId(Id locationId) const;

  [[nodiscard]] Id getFirstActiveLocationId(Id tokenId) const;
  [[nodiscard]] QRectF getLineRectForLineNumber(size_t lineNumber) const;

  [[nodiscard]] std::string getCode() const;

  void findScreenMatches(const std::wstring& query, std::vector<std::pair<QtCodeArea*, Id>>* screenMatches);

  [[nodiscard]] bool hasFocus(const CodeFocusHandler::Focus& focus) const;
  bool setFocus(Id locationId);
  bool moveFocus(const CodeFocusHandler::Focus& focus, CodeFocusHandler::Direction direction);
  void focusTop();
  void focusBottom();

  void copySelection();

  void ensureLocationIdVisible(Id locationId, bool animated);

private slots:
  void clickedTitle();
  void clickedFooter();

private:
  QtHoverButton* createScopeLine(QBoxLayout* layout);
  void updateDots();
  void updateScopeLineFocus(QPushButton* line, QPushButton* dots);

  QtCodeNavigator* m_navigator;
  QtCodeFile* m_file;

  Id m_titleId;
  std::wstring m_titleString;

  Id m_footerId;
  std::wstring m_footerString;

  std::vector<QPushButton*> m_dots;

  QtHoverButton* m_title = nullptr;
  QtHoverButton* m_footer = nullptr;

  QPushButton* m_titleDots = nullptr;
  QPushButton* m_footerDots = nullptr;

  QtCodeArea* m_codeArea = nullptr;
};