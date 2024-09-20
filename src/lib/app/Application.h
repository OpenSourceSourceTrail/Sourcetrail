#pragma once
// STL
#include <cassert>
#include <memory>
// internal
#include "DialogView.h"
#include "FilePath.h"
#include "Project.h"
// messages
#include "MessageActivateWindow.h"
#include "MessageBookmarkUpdate.hpp"
#include "MessageCloseProject.h"
#include "MessageIndexingFinished.h"
#include "MessageListener.h"
#include "MessageLoadProject.h"
#include "MessageRefresh.h"
#include "MessageRefreshUI.h"
#include "MessageSwitchColorScheme.h"

class Bookmark;
class IDECommunicationController;
class MainView;
class NetworkFactory;
class StorageCache;
class UpdateChecker;
class Version;
class ViewFactory;

class Application final
    : public MessageListener<MessageActivateWindow>
    , public MessageListener<MessageCloseProject>
    , public MessageListener<MessageIndexingFinished>
    , public MessageListener<MessageLoadProject>
    , public MessageListener<MessageRefresh>
    , public MessageListener<MessageRefreshUI>
    , public MessageListener<MessageSwitchColorScheme>
    , public MessageListener<MessageBookmarkUpdate> {
public:
  /**
   * @brief Creates the singleton instance of the Application.
   * @param version The version of the application.
   * @param viewFactory Pointer to the ViewFactory.
   * @param networkFactory Pointer to the NetworkFactory.
   */
  static void createInstance(const Version& version, ViewFactory* viewFactory, NetworkFactory* networkFactory);

  /**
   * @brief Gets the singleton instance of the Application.
   * @return Shared pointer to the Application instance.
   */
  static std::shared_ptr<Application> getInstance() {
    assert(s_instance);
    return s_instance;
  }

  /**
   * @brief Destroys the singleton instance of the Application.
   */
  static void destroyInstance();

  /**
   * @brief Gets the UUID of the application.
   * @return The UUID as a string.
   */
  static std::string getUUID();

  /**
   * @brief Loads the application settings.
   */
  static void loadSettings();

  /**
   * @brief Loads the application style from a color scheme file.
   * @param colorSchemePath The path to the color scheme file.
   */
  static void loadStyle(const FilePath& colorSchemePath);

  /**
   * @brief Destructor for the Application class.
   */
  ~Application() override;

  // Disable copy and move operators
  Application(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(const Application&) = delete;
  Application& operator=(Application&&) = delete;

  /**
   * @brief Gets the current project.
   * @return Shared pointer to the current Project.
   */
  [[nodiscard]] std::shared_ptr<const Project> getCurrentProject() const noexcept {
    return m_pProject;
  }

  /**
   * @brief Gets the path of the current project.
   * @return FilePath of the current project.
   */
  [[nodiscard]] FilePath getCurrentProjectPath() const noexcept {
    return m_pProject ? m_pProject->getProjectSettingsFilePath() : FilePath{};
  }

  /**
   * @brief Checks if a project is currently loaded.
   * @return True if a project is loaded, false otherwise.
   */
  [[nodiscard]] bool isProjectLoaded() const noexcept {
    return m_pProject ? m_pProject->isLoaded() : false;
  }

  /**
   * @brief Checks if the application has a GUI.
   * @return True if the application has a GUI, false otherwise.
   */
  [[nodiscard]] bool hasGUI() const noexcept { return m_hasGUI; }

  /**
   * @brief Handles a dialog with a message.
   * @param message The message to display in the dialog.
   * @return The result of the dialog interaction.
   */
  int handleDialog(const std::wstring& message);

  /**
   * @brief Handles a dialog with a message and options.
   * @param message The message to display in the dialog.
   * @param options The options to present in the dialog.
   * @return The result of the dialog interaction.
   */
  int handleDialog(const std::wstring& message, const std::vector<std::wstring>& options);

  /**
   * @brief Gets a dialog view for a specific use case.
   * @param useCase The use case for the dialog.
   * @return Shared pointer to the DialogView.
   */
  std::shared_ptr<DialogView> getDialogView(DialogView::UseCase useCase);

  /**
   * @brief Updates the history menu.
   * @param message The message containing the update information.
   */
  void updateHistoryMenu(std::shared_ptr<MessageBase> message);

private:
  static std::shared_ptr<Application> s_instance;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static std::string s_uuid;                         // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  explicit Application(bool withGUI = true);

  /**
   * @name handle Message group
   * @{ */
  void handleMessage(MessageActivateWindow* pMessage) override;
  void handleMessage(MessageCloseProject* pMessage) override;
  void handleMessage(MessageIndexingFinished* pMessage) override;
  /**
   * @brief Handle a load project message
   *
   * @param pMessage a message
   */
  void handleMessage(MessageLoadProject* pMessage) override;
  void handleMessage(MessageRefresh* pMessage) override;
  void handleMessage(MessageRefreshUI* pMessage) override;
  void handleMessage(MessageSwitchColorScheme* pMessage) override;
  /**  @} */
  void handleMessage(MessageBookmarkUpdate* message) override;

  void startMessagingAndScheduling();

  void loadWindow(bool showStartWindow);

  void refreshProject(RefreshMode refreshMode, bool shallowIndexingRequested);
  void updateRecentProjects(const FilePath& projectSettingsFilePath);

  void logStorageStats() const;

  void updateTitle();

  bool checkSharedMemory();

  const bool m_hasGUI;
  bool m_loadedWindow = false;

  std::shared_ptr<Project> m_pProject;
  std::shared_ptr<StorageCache> m_storageCache;

  std::shared_ptr<MainView> m_mainView;

  std::shared_ptr<IDECommunicationController> m_ideCommunicationController;
};