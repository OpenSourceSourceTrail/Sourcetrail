#include "QtProjectWizardContentPathsFrameworkSearchGlobal.h"

#include <range/v3/to_container.hpp>
#include <range/v3/view/transform.hpp>

#include "globalStrings.h"
#include "IApplicationSettings.hpp"
#include "utility.h"
#include "utilityPathDetection.h"

QtProjectWizardContentPathsFrameworkSearchGlobal::QtProjectWizardContentPathsFrameworkSearchGlobal(QtProjectWizardWindow* window)
    : QtProjectWizardContentPaths(
          std::shared_ptr<SourceGroupSettings>(), window, QtPathListBox::SELECTION_POLICY_DIRECTORIES_ONLY, true) {
  setTitleString(QStringLiteral("Global Framework Search Paths"));
  setHelpString(QString::fromStdString(
      std::string{"The Global Framework Search Paths will be used in all your projects - in addition to the "
                  "project specific "
                  "Framework Search Paths.<br />"
                  "<br />"
                  "They define where MacOS framework containers (.framework) are found "
                  "(See <a href=\""} +
      "documentation"_g +
      "#finding-system-header-locations\">"
      "Finding System Header Locations</a> or use the auto detection below)."));

  m_pathDetector = utility::getCxxFrameworkPathDetector();
  m_makePathsRelativeToProjectFileLocation = false;
}

void QtProjectWizardContentPathsFrameworkSearchGlobal::load() {
  m_list->setPaths(utility::toFilePath(IApplicationSettings::getInstanceRaw()->getFrameworkSearchPaths()));
}

void QtProjectWizardContentPathsFrameworkSearchGlobal::save() {
  IApplicationSettings::getInstanceRaw()->setFrameworkSearchPaths(utility::toStlPath(m_list->getPathsAsDisplayed()));
  IApplicationSettings::getInstanceRaw()->save();
}
