#pragma once
#include <functional>
#include <vector>

#include "code/CodeSnapshot.h"
#include "component/view/CodeView.h"

namespace code {

/**
 * The CodeView the controller pushes snippets into.
 *
 * Write-only, like every view here except the search one: each call flattens what it is handed into
 * a CodeSnapshot and passes it to the handler, which posts it to the GUI thread. Nothing in here is
 * read back from the bus thread, so no locking is needed.
 */
class QmlCodeView final : public CodeView {
public:
  using SnapshotHandler = std::function<void(CodeSnapshot)>;
  using ScrollHandler = std::function<void(CodeScrollParams, bool animated)>;

  QmlCodeView(SnapshotHandler onSnapshot, ScrollHandler onScroll);
  ~QmlCodeView() override;

  void refreshView() override;

  void clear() override;

  void showSnippets(const std::vector<CodeFileParams>& files, const CodeParams& params, const CodeScrollParams& scrollParams) override;

  void showSingleFile(const CodeFileParams& file, const CodeParams& params, const CodeScrollParams& scrollParams) override;

  void updateSourceLocations(const std::vector<CodeFileParams>& files) override;

  void scrollTo(const CodeScrollParams& params, bool animated) override;

  [[nodiscard]] bool showsErrors() const override;

  void coFocusTokenIds(const std::vector<Id>& coFocusedTokenIds) override;
  void deCoFocusTokenIds() override;

  [[nodiscard]] bool isInListMode() const override;
  void setMode(bool listMode) override;

  [[nodiscard]] bool hasSingleFileCached(const FilePath& filePath) const override;

  void setNavigationFocus(bool focus) override;
  [[nodiscard]] bool hasNavigationFocus() const override;

  /** @name ScreenSearchResponder. CodeView already supplies getName(). @{ */
  [[nodiscard]] bool isVisible() const override;
  void findMatches(ScreenSearchSender* sender, const std::wstring& query) override;
  void activateMatch(size_t matchIndex) override;
  void deactivateMatch(size_t matchIndex) override;
  void clearMatches() override;
  /** @} */

private:
  SnapshotHandler mOnSnapshot;
  ScrollHandler mOnScroll;

  bool mListMode = true;
  bool mShowsErrors = false;
  bool mHasNavigationFocus = false;

  /** The paths currently on screen, so hasSingleFileCached() can answer without the GUI thread. */
  std::vector<FilePath> mSingleFilePaths;

  /**
   * The params the current snippets were built with.
   *
   * updateSourceLocations() is handed locations but not params, and which locations count as active
   * or focused lives in the params. Rebuilding from a default-constructed set would mark every
   * location inactive, so the decoration pass would find nothing to paint and the active symbol
   * would lose its highlight the moment locations refreshed.
   */
  CodeParams mParams;
};

}    // namespace code
