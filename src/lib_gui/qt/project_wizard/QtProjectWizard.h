#pragma once
#include <functional>

#include "QtProjectWizardWindow.h"
#include "QtWindowStack.h"
#include "SourceGroupType.h"
#include "details/ApplicationSettings.h"

class QWidget;
class QListWidget;
class QPushButton;
class QtProjectWizardContent;

class SourceGroupSettings;
class ProjectSettings;
class ProjectWizardModel;

class QtProjectWizard : public QtProjectWizardWindow {
  // NOLINTNEXTLINE(readability-identifier-length)
  Q_OBJECT

public:
  explicit QtProjectWizard(QWidget* pParent = nullptr);

  QSize sizeHint() const override;

  // public slots:
  void newProject();

  void newProjectFromCDB(const FilePath& filePath);

  void editProject(const FilePath& settingsPath);
  void editProject(std::shared_ptr<ProjectSettings> settings);

protected:
  void populateWindow(QWidget* widget) override;
  void windowReady() override;

  void handlePrevious() override;

private:
  QtProjectWizardWindow* createWindowWithContent(std::function<QtProjectWizardContent*(QtProjectWizardWindow*)> func);

  void updateSourceGroupList();
  bool canExitContent();

  QtWindowStack m_windowStack;

  // TODO: They should be moved to `ProjectWizardModel`
  std::shared_ptr<ProjectSettings> m_projectSettings;
  std::vector<std::shared_ptr<SourceGroupSettings>> m_allSourceGroupSettings;
  ApplicationSettings m_appSettings;

  bool m_editing = false;
  int m_previouslySelectedIndex = -1;

  QPushButton* m_generalButton = nullptr;
  QPushButton* m_removeButton = nullptr;
  QPushButton* m_duplicateButton = nullptr;
  QListWidget* m_sourceGroupList = nullptr;
  QWidget* m_contentWidget = nullptr;

  std::shared_ptr<ProjectWizardModel> m_model;

  // private slots:
  void generalButtonClicked();
  void selectedSourceGroupChanged(int index);
  void selectedSourceGroupNameChanged(const QString& name);
  void removeSelectedSourceGroup();
  void duplicateSelectedSourceGroup();

  void cancelSourceGroup();

  void cancelWizard();
  void finishWizard();

  void windowStackChanged();

  void newSourceGroup();
  void newSourceGroupFromVS();
  void selectedProjectType(SourceGroupType sourceGroupType);

  void createSourceGroup(std::shared_ptr<SourceGroupSettings> settings);
  void createProject();
};