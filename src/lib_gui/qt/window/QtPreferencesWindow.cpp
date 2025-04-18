#include "QtPreferencesWindow.h"

#include "Application.h"
#include "DialogView.h"
#include "language_packages.h"
#include "QtProjectWizardContentGroup.h"
#include "QtProjectWizardContentPathsFrameworkSearchGlobal.h"
#include "QtProjectWizardContentPathsHeaderSearchGlobal.h"
#include "QtProjectWizardContentPreferences.h"
#include "type/MessageLoadProject.h"
#include "type/MessageRefreshUI.h"
#include "type/MessageScrollSpeedChange.h"
#include "type/plugin/MessagePluginPortChange.h"
#include "utilityApp.h"

QtPreferencesWindow::QtPreferencesWindow(QWidget* parent) : QtProjectWizardWindow(parent) {
  // save old application settings so they can be compared later
  IApplicationSettings* appSettings = IApplicationSettings::getInstanceRaw();
  m_appSettings.setHeaderSearchPaths(appSettings->getHeaderSearchPaths());
  m_appSettings.setFrameworkSearchPaths(appSettings->getFrameworkSearchPaths());
  m_appSettings.setScrollSpeed(appSettings->getScrollSpeed());
  m_appSettings.setSourcetrailPort(appSettings->getSourcetrailPort());
  m_appSettings.setPluginPort(appSettings->getPluginPort());

  m_appSettings.setScreenAutoScaling(appSettings->getScreenAutoScaling());
  m_appSettings.setScreenScaleFactor(appSettings->getScreenScaleFactor());


  QtProjectWizardContentGroup* summary = new QtProjectWizardContentGroup(this);
  summary->addContent(new QtProjectWizardContentPreferences(this));

#if BUILD_CXX_LANGUAGE_PACKAGE
  summary->addContent(new QtProjectWizardContentPathsHeaderSearchGlobal(this));
  if(utility::getOsType() == OsType::Mac) {
    summary->addContent(new QtProjectWizardContentPathsFrameworkSearchGlobal(this));
  }
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

  setPreferredSize(QSize(750, 500));
  setContent(summary);
  setScrollAble(true);
}

QtPreferencesWindow::~QtPreferencesWindow() = default;

void QtPreferencesWindow::windowReady() {
  QtProjectWizardWindow::windowReady();

  updateTitle(QStringLiteral("PREFERENCES"));
  updateNextButton(QStringLiteral("Save"));
  setPreviousVisible(false);

  loadContent();
}

void QtPreferencesWindow::handleNext() {
  if(!content()->check()) {
    return;
  }

  saveContent();

  Application* app = Application::getInstance().get();
  IApplicationSettings* appSettings = IApplicationSettings::getInstanceRaw();

  bool needsRestart = m_appSettings.getScreenAutoScaling() != appSettings->getScreenAutoScaling() ||
      m_appSettings.getScreenScaleFactor() != appSettings->getScreenScaleFactor();

  if(needsRestart) {
    app->getDialogView(DialogView::UseCase::PROJECT_SETUP)
        ->confirm(
            L"<p>Please restart the application for all changes to take effect.</p><p>Note: "
            L"These changes may harm "
            L"the execution of the application. In case the application is not useable "
            L"anymore, please run the "
            L"'resetPreferences.sh' script located in your install directory.</p>");
  }


  bool appSettingsChanged = !(m_appSettings == *appSettings);

  if(m_appSettings.getScrollSpeed() != appSettings->getScrollSpeed()) {
    MessageScrollSpeedChange(appSettings->getScrollSpeed()).dispatch();
  }

  if(m_appSettings.getSourcetrailPort() != appSettings->getSourcetrailPort() ||
     m_appSettings.getPluginPort() != appSettings->getPluginPort()) {
    MessagePluginPortChange().dispatch();
  }

  app->loadSettings();

  if(appSettingsChanged) {
    auto currentProject = Application::getInstance()->getCurrentProject();
    if(currentProject) {
      MessageLoadProject(currentProject->getProjectSettingsFilePath(), true).dispatch();
    }
  } else {
    MessageRefreshUI().dispatch();
  }

  QtWindow::handleClose();
}
