#include "QtProjectWizardContentPathsHeaderSearchGlobal.h"

#include <QMessageBox>

#include "globalStrings.h"
#include "IApplicationSettings.hpp"
#include "ResourcePaths.h"
#include "utility.h"
#include "utilityApp.h"
#include "utilityPathDetection.h"

QtProjectWizardContentPathsHeaderSearchGlobal::QtProjectWizardContentPathsHeaderSearchGlobal(QtProjectWizardWindow* window)
    : QtProjectWizardContentPaths(
          std::shared_ptr<SourceGroupSettings>(), window, QtPathListBox::SELECTION_POLICY_DIRECTORIES_ONLY, true) {
  setTitleString(QStringLiteral("Global Include Paths"));
  setHelpString(
      QString::fromStdString(std::string{"The Global Include Paths will be used in all your projects in addition to the project "
                                         "specific Include Paths. "
                                         "These paths are usually passed to the compiler with the '-isystem' flag.<br />"
                                         "<br />"
                                         "Use them to add system header paths (See <a "
                                         "href=\""} +
                             "documentation"_g +
                             "#finding-system-header-locations\">"
                             "Finding System Header Locations</a> or use the auto detection below)."));

  m_pathDetector = utility::getCxxHeaderPathDetector();
  m_makePathsRelativeToProjectFileLocation = false;
}

void QtProjectWizardContentPathsHeaderSearchGlobal::load() {
  setPaths(IApplicationSettings::getInstanceRaw()->getHeaderSearchPaths());
}

void QtProjectWizardContentPathsHeaderSearchGlobal::save() {
  std::vector<FilePath> paths;
  for(const FilePath& headerPath : m_list->getPathsAsDisplayed()) {
    if(headerPath != ResourcePaths::getCxxCompilerHeaderDirectoryPath()) {
      paths.push_back(headerPath);
    }
  }

  IApplicationSettings::getInstanceRaw()->setHeaderSearchPaths(utility::toStlPath(paths));
  IApplicationSettings::getInstanceRaw()->save();
}

bool QtProjectWizardContentPathsHeaderSearchGlobal::check() {
  if constexpr(utility::getOsType() == OsType::Windows) {
    return QtProjectWizardContentPaths::check();
  }

  std::vector<FilePath> paths;
  QString compilerHeaderPaths;
  for(const FilePath& headerPath : m_list->getPathsAsDisplayed()) {
    if(headerPath != ResourcePaths::getCxxCompilerHeaderDirectoryPath() &&
       headerPath.getCanonical().getConcatenated(L"/stdarg.h").exists()) {
      compilerHeaderPaths += QString::fromStdWString(headerPath.wstr()) + "\n";
    } else {
      paths.push_back(headerPath);
    }
  }

  if(compilerHeaderPaths.size()) {
    QMessageBox msgBox(mWindow);
    msgBox.setText(QStringLiteral("Multiple Compiler Headers"));
    msgBox.setInformativeText(
        "Your Global Include Paths contain other paths that hold C/C++ compiler headers, "
        "probably those of your local C/C++ compiler. They are possibly in conflict with the "
        "compiler headers of "
        "Sourcetrail's C/C++ indexer. This can lead to compatibility errors during indexing. "
        "Do "
        "you want to remove "
        "these paths?");
    msgBox.setDetailedText(compilerHeaderPaths);
    msgBox.addButton(QStringLiteral("Remove"), QMessageBox::ButtonRole::YesRole);
    msgBox.addButton(QStringLiteral("Keep"), QMessageBox::ButtonRole::NoRole);
    msgBox.setIcon(QMessageBox::Icon::Question);
    int ret = msgBox.exec();

    if(ret == 0)    // QMessageBox::Yes
    {
      setPaths(utility::toStlPath(paths));
      save();
    }
  }

  return QtProjectWizardContentPaths::check();
}

void QtProjectWizardContentPathsHeaderSearchGlobal::detectedPaths(const std::vector<FilePath>& paths) {
  std::vector<FilePath> headerPaths;
  for(const FilePath& headerPath : paths) {
    if(headerPath != ResourcePaths::getCxxCompilerHeaderDirectoryPath()) {
      headerPaths.push_back(headerPath);
    }
  }
  setPaths(utility::toStlPath(headerPaths));
}

void QtProjectWizardContentPathsHeaderSearchGlobal::setPaths(const std::vector<std::filesystem::path>& paths) {
  // check data change to avoid UI update that messes with the scroll position
  {
    auto currentPaths = utility::toStlPath(m_list->getPathsAsDisplayed());
    if(!currentPaths.empty()) {
      currentPaths.erase(currentPaths.begin());
    }

    if(currentPaths.size() == paths.size()) {
      bool same = true;
      for(size_t i = 0; i < paths.size(); ++i) {
        if(currentPaths[i] != paths[i]) {
          same = false;
          break;
        }
      }

      if(same) {
        return;
      }
    }
  }

  m_list->setPaths({});
  m_list->addPaths({ResourcePaths::getCxxCompilerHeaderDirectoryPath()}, true);
  m_list->addPaths(utility::toFilePath(paths));
}
