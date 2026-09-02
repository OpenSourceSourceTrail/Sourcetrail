#include "project/ui/content/paths/QtProjectWizardContentPathsHeaderSearch.h"

#include <QGridLayout>
#include <QMessageBox>
#include <QtConcurrent>

#include "app/Application.h"
#include "project/logic/HeaderSearchDetection.h"
#include "qt/view/QtDialogView.h"
#include "qt/window/QtPathListDialog.h"
#include "qt/window/QtTextEditDialog.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPathsAndFlags.h"
#include "settings/source_group/component/SourceGroupSettingsWithSourcePaths.h"
#include "settings/source_group/SourceGroupSettings.h"
#include "utility.h"
#include "utility/IncludeDirective.h"
#include "utilityFile.h"

QtProjectWizardContentPathsHeaderSearch::QtProjectWizardContentPathsHeaderSearch(std::shared_ptr<SourceGroupSettings> settings,
                                                                                 QtProjectWizardWindow* window,
                                                                                 bool indicateAsAdditional)
    : QtProjectWizardContentPaths(settings, window, QtPathListBox::SELECTION_POLICY_DIRECTORIES_ONLY, true)
    , m_indicateAsAdditional(indicateAsAdditional) {
  // Both detections deliver on the GUI thread, so the result handlers need no marshalling of their
  // own -- QFutureWatcher::finished is emitted here, not on the worker.
  connect(&m_validationWatcher, &QFutureWatcher<std::vector<IncludeDirective>>::finished, this, [this] {
    endDetection();
    showValidationResult(m_validationWatcher.result());
  });
  connect(&m_detectionWatcher, &QFutureWatcher<std::set<FilePath>>::finished, this, [this] {
    endDetection();
    showDetectedIncludesResult(m_detectionWatcher.result());
  });

  setTitleString(m_indicateAsAdditional ? "Additional Include Paths" : "Include Paths");
  setHelpString(((m_indicateAsAdditional ? "<b>Note</b>: Use the Additional Include Paths to add paths "
                                           "that are missing in the "
                                           "referenced project file.<br /><br />" :
                                           "") +
                 std::string("Include Paths are used for resolving #include directives in the indexed "
                             "source and header files. These paths are "
                             "usually passed to the compiler with the '-I' or '-iquote' flags.<br />"
                             "<br />"
                             "Add all paths #include directives throughout your project are relative to. "
                             "If all #include directives are "
                             "specified relative to the project's root directory, please add that root "
                             "directory here.<br />"
                             "<br />"
                             "If your project also includes files from external libraries (e.g. boost), "
                             "please add these directories as well "
                             "(e.g. add 'path/to/boost_home/include').<br />"
                             "<br />"
                             "You can make use of environment variables with ${ENV_VAR}."))
                    .c_str());
}

void QtProjectWizardContentPathsHeaderSearch::populate(QGridLayout* layout, int& row) {
  QtProjectWizardContentPaths::populate(layout, row);

  if(!m_indicateAsAdditional) {
    {
      QPushButton* detectionButton = new QPushButton(QStringLiteral("auto-detect"));
      detectionButton->setObjectName(QStringLiteral("windowButton"));
      connect(detectionButton, &QPushButton::clicked, this, &QtProjectWizardContentPathsHeaderSearch::detectIncludesButtonClicked);
      layout->addWidget(detectionButton, row, QtProjectWizardWindow::BACK_COL, Qt::AlignLeft | Qt::AlignTop);
    }
    {
      QPushButton* validationButton = new QPushButton(QStringLiteral("validate include directives"));
      validationButton->setObjectName(QStringLiteral("windowButton"));
      connect(
          validationButton, &QPushButton::clicked, this, &QtProjectWizardContentPathsHeaderSearch::validateIncludesButtonClicked);
      layout->addWidget(validationButton, row, QtProjectWizardWindow::BACK_COL, Qt::AlignRight | Qt::AlignTop);
    }
    row++;
  }
}

void QtProjectWizardContentPathsHeaderSearch::load() {
  std::shared_ptr<SourceGroupSettingsWithCxxPathsAndFlags> cxxSettings =
      std::dynamic_pointer_cast<SourceGroupSettingsWithCxxPathsAndFlags>(m_settings);
  if(cxxSettings) {
    m_list->setPaths(cxxSettings->getHeaderSearchPaths());
  }
}

void QtProjectWizardContentPathsHeaderSearch::save() {
  std::shared_ptr<SourceGroupSettingsWithCxxPathsAndFlags> cxxSettings =
      std::dynamic_pointer_cast<SourceGroupSettingsWithCxxPathsAndFlags>(m_settings);
  if(cxxSettings) {
    cxxSettings->setHeaderSearchPaths(m_list->getPathsAsDisplayed());
  }
}

void QtProjectWizardContentPathsHeaderSearch::detectIncludesButtonClicked() {
  m_window->saveContent();

  m_pathsDialog = std::make_shared<QtPathListDialog>(
      "Detect Include Paths",
      "<p>Automatically search for header files in the paths provided below to find missing "
      "include paths for "
      "unresolved include directives in your source code.</p>"
      "<p>The \"Files & Directories to Index\" will be searched by default, but you can add "
      "further paths, such "
      "as directories of third-party libraries, if required.</p>",
      QtPathListBox::SELECTION_POLICY_DIRECTORIES_ONLY);

  m_pathsDialog->setup();
  m_pathsDialog->updateNextButton(QStringLiteral("Start"));
  m_pathsDialog->setCloseVisible(true);

  m_pathsDialog->setRelativeRootDirectory(m_settings->getProjectDirectoryPath());
  if(std::shared_ptr<SourceGroupSettingsWithSourcePaths> pathSettings =
         std::dynamic_pointer_cast<SourceGroupSettingsWithSourcePaths>(m_settings))    // FIXME: pass msettings as required type
  {
    m_pathsDialog->setPaths(pathSettings->getSourcePaths(), true);
  }
  m_pathsDialog->showWindow();

  connect(m_pathsDialog.get(),
          &QtPathListDialog::finished,
          this,
          &QtProjectWizardContentPathsHeaderSearch::finishedSelectDetectIncludesRootPathsDialog);
  connect(m_pathsDialog.get(), &QtPathListDialog::canceled, this, &QtProjectWizardContentPathsHeaderSearch::closedPathsDialog);
}

std::optional<std::pair<utility::HeaderDetectionInputs, utility::DetectionProgress>> QtProjectWizardContentPathsHeaderSearch::beginDetection() {
  std::optional<utility::HeaderDetectionInputs> inputs = utility::collectDetectionInputs(m_settings);
  if(!inputs) {
    return std::nullopt;
  }

  m_dialogView = dynamic_cast<QtDialogView*>(Application::getInstance()->getDialogView(DialogView::UseCase::PROJECT_SETUP).get());
  m_dialogView->setParentWindow(m_window);

  // QtDialogView marshals every one of these to the Qt thread itself, so the worker may call it.
  utility::DetectionProgress progress = [dialogView = m_dialogView](const std::wstring& message, std::optional<size_t> percent) {
    if(percent) {
      dialogView->hideUnknownProgressDialog();
      dialogView->showProgressDialog(L"Processing", message, *percent);
    } else {
      dialogView->showUnknownProgressDialog(L"Processing", message);
    }
  };

  return std::make_pair(std::move(*inputs), std::move(progress));
}

void QtProjectWizardContentPathsHeaderSearch::endDetection() {
  if(m_dialogView != nullptr) {
    // Both, because a source tree with no files never reaches the determinate dialog.
    m_dialogView->hideUnknownProgressDialog();
    m_dialogView->hideProgressDialog();
    m_dialogView = nullptr;
  }
}

void QtProjectWizardContentPathsHeaderSearch::validateIncludesButtonClicked() {
  // TODO: regard Force Includes here, too!
  m_window->saveContent();

  if(auto detection = beginDetection()) {
    m_validationWatcher.setFuture(QtConcurrent::run(utility::findUnresolvedIncludes, detection->first, detection->second));
  }
}

void QtProjectWizardContentPathsHeaderSearch::finishedSelectDetectIncludesRootPathsDialog() {
  // TODO: regard Force Includes here, too!
  const std::set<FilePath> searchedPaths = utility::toSet(m_settings->makePathsExpandedAndAbsolute(m_pathsDialog->getPaths()));
  closedPathsDialog();

  if(auto detection = beginDetection()) {
    m_detectionWatcher.setFuture(
        QtConcurrent::run(utility::detectHeaderSearchDirectories, detection->first, searchedPaths, detection->second));
  }
}

void QtProjectWizardContentPathsHeaderSearch::finishedAcceptDetectedIncludePathsDialog() {
  const std::vector<std::wstring> detectedPaths = utility::split<std::vector<std::wstring>>(m_filesDialog->getText(), L"\n");
  closedFilesDialog();

  std::vector<FilePath> headerSearchPaths = m_list->getPathsAsDisplayed();

  headerSearchPaths.reserve(headerSearchPaths.size() + detectedPaths.size());
  for(const std::wstring& detectedPath : detectedPaths) {
    if(!detectedPath.empty()) {
      headerSearchPaths.push_back(FilePath(detectedPath));
    }
  }

  m_list->setPaths(headerSearchPaths);
}

void QtProjectWizardContentPathsHeaderSearch::closedPathsDialog() {
  m_pathsDialog->hide();
  m_pathsDialog.reset();

  window()->raise();
}

void QtProjectWizardContentPathsHeaderSearch::showDetectedIncludesResult(const std::set<FilePath>& detectedHeaderSearchPaths) {
  const std::set<FilePath> headerSearchPaths = utility::toSet(
      m_settings->makePathsExpandedAndAbsolute(m_list->getPathsAsDisplayed()));

  std::vector<FilePath> additionalHeaderSearchPaths;
  {
    const FilePath relativeRoot = m_list->getRelativeRootDirectory();
    for(const FilePath& detectedHeaderSearchPath : detectedHeaderSearchPaths) {
      if(headerSearchPaths.find(detectedHeaderSearchPath) == headerSearchPaths.end()) {
        additionalHeaderSearchPaths.push_back(utility::getAsRelativeIfShorter(detectedHeaderSearchPath, relativeRoot));
      }
    }
  }

  if(additionalHeaderSearchPaths.empty()) {
    QMessageBox msgBox(m_window);
    msgBox.setText(
        "<p>No additional include paths have been detected while searching the provided "
        "paths.</p>");
    msgBox.exec();
  } else {
    std::wstring detailedText = L"";
    for(const FilePath& path : additionalHeaderSearchPaths) {
      detailedText += path.wstr() + L"\n";
    }

    m_filesDialog = new QtTextEditDialog("Detected Include Paths",
                                         ("<p>The following <b>" + std::to_string(additionalHeaderSearchPaths.size()) +
                                          "</b> include paths have been "
                                          "detected and will be added to the include paths of this Source Group.<b>")
                                             .c_str(),
                                         m_window);

    m_filesDialog->setup();
    m_filesDialog->setReadOnly(true);
    m_filesDialog->setCloseVisible(true);
    m_filesDialog->updateNextButton(QStringLiteral("Add"));

    m_filesDialog->setText(detailedText);
    m_filesDialog->showWindow();

    connect(m_filesDialog,
            &QtTextEditDialog::finished,
            this,
            &QtProjectWizardContentPathsHeaderSearch::finishedAcceptDetectedIncludePathsDialog);
    connect(m_filesDialog, &QtTextEditDialog::canceled, this, &QtProjectWizardContentPathsHeaderSearch::closedFilesDialog);
  }
}

void QtProjectWizardContentPathsHeaderSearch::showValidationResult(const std::vector<IncludeDirective>& unresolvedIncludes) {
  if(unresolvedIncludes.empty()) {
    QMessageBox msgBox(m_window);
    msgBox.setText(QStringLiteral("<p>All include directives throughout the indexed files have been resolved.</p>"));
    msgBox.exec();
  } else {
    std::map<std::wstring, std::map<size_t, std::wstring>> orderedIncludes;
    for(const IncludeDirective& unresolvedInclude : unresolvedIncludes) {
      orderedIncludes[unresolvedInclude.getIncludingFile().wstr()].emplace(
          unresolvedInclude.getLineNumber(), unresolvedInclude.getDirective());
    }

    std::wstring detailedText = L"";
    for(const auto& p : orderedIncludes) {
      detailedText += p.first + L"\n";

      for(const auto& p2 : p.second) {
        detailedText += std::to_wstring(p2.first) + L":\t" + p2.second + L"\n";
      }

      detailedText += L"\n";
    }

    m_filesDialog = new QtTextEditDialog("Unresolved Include Directives",
                                         ("<p>The indexed files contain <b>" + std::to_string(unresolvedIncludes.size()) +
                                          "</b> include directive" + (unresolvedIncludes.size() == 1 ? "" : "s") +
                                          " that could not be resolved correctly. Please check the details "
                                          "and add the respective header search paths.</p>"
                                          "<p><b>Note</b>: This is only a quick pass that does not regard block commenting or "
                                          "conditional preprocessor "
                                          "directives. This means that some of the unresolved includes may actually not be "
                                          "required by the indexer.</p>")
                                             .c_str(),
                                         m_window);

    m_filesDialog->setup();
    m_filesDialog->setCloseVisible(false);
    m_filesDialog->setReadOnly(true);

    m_filesDialog->setText(detailedText);
    m_filesDialog->showWindow();

    connect(m_filesDialog, &QtTextEditDialog::finished, this, &QtProjectWizardContentPathsHeaderSearch::closedFilesDialog);
    connect(m_filesDialog, &QtTextEditDialog::canceled, this, &QtProjectWizardContentPathsHeaderSearch::closedFilesDialog);
  }
}
