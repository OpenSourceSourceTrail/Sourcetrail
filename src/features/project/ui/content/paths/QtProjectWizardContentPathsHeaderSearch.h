#ifndef QT_PROJECT_WIZARD_CONTENT_PATHS_HEADER_SEARCH_H
#define QT_PROJECT_WIZARD_CONTENT_PATHS_HEADER_SEARCH_H

#include <optional>
#include <utility>

#include <QFutureWatcher>

#include "FilePathFilter.h"
#include "project/logic/HeaderSearchDetection.h"
#include "project/ui/content/paths/QtProjectWizardContentPaths.h"

class QtDialogView;
class QtPathListDialog;

class QtProjectWizardContentPathsHeaderSearch : public QtProjectWizardContentPaths {
  Q_OBJECT
public:
  QtProjectWizardContentPathsHeaderSearch(std::shared_ptr<SourceGroupSettings> settings,
                                          QtProjectWizardWindow* window,
                                          bool indicateAsAdditional = false);

  // QtProjectWizardContent implementation
  virtual void populate(QGridLayout* layout, int& row) override;
  virtual void load() override;
  virtual void save() override;

private slots:
  void detectIncludesButtonClicked();
  void validateIncludesButtonClicked();
  void finishedSelectDetectIncludesRootPathsDialog();
  void finishedAcceptDetectedIncludePathsDialog();
  void closedPathsDialog();

private:
  void showDetectedIncludesResult(const std::set<FilePath>& detectedHeaderSearchPaths);
  void showValidationResult(const std::vector<IncludeDirective>& unresolvedIncludes);

  /**
   * Reads the settings and opens the progress dialog, on the GUI thread.
   *
   * Nullopt when this source group carries no source paths, so there is nothing to detect. Both
   * halves have to happen here: the settings are the ones the wizard is still editing, and the
   * dialog comes from Application, which a worker thread has no business reaching for.
   */
  std::optional<std::pair<utility::HeaderDetectionInputs, utility::DetectionProgress>> beginDetection();

  /** Closes whichever progress dialog `beginDetection` left open. */
  void endDetection();

  QFutureWatcher<std::vector<IncludeDirective>> m_validationWatcher;
  QFutureWatcher<std::set<FilePath>> m_detectionWatcher;
  QtDialogView* m_dialogView = nullptr;

  std::shared_ptr<QtPathListDialog> m_pathsDialog;
  const bool m_indicateAsAdditional;
};

#endif    // QT_PROJECT_WIZARD_CONTENT_PATHS_HEADER_SEARCH_H
