#pragma once
// STL
#include <cassert>
#include <memory>

#include "DialogView.h"
#include "FilePath.h"
#include "Project.h"
// messages
#include "MessageListener.h"
#include "type/bookmark/MessageBookmarkUpdate.hpp"
#include "type/indexing/MessageIndexingFinished.h"
#include "type/MessageActivateWindow.h"
#include "type/MessageCloseProject.h"
#include "type/MessageLoadProject.h"
#include "type/MessageRefresh.h"
#include "type/MessageRefreshUI.h"
#include "type/MessageSwitchColorScheme.h"

class Bookmark;
class IDECommunicationController;
class MainView;
class NetworkFactory;
class StorageCache;
class Version;
class ViewFactory;
namespace lib {
struct IFactory;
}

class Application final
    : public MessageListener<MessageActivateWindow>
    , public MessageListener<MessageCloseProject>
    , public MessageListener<MessageIndexingFinished>
    , public MessageListener<MessageLoadProject>
#if !defined(SOURCETRAIL_WASM)
    , public MessageListener<MessageRefresh>
#endif
    , public MessageListener<MessageRefreshUI>
    , public MessageListener<MessageSwitchColorScheme>
    , public MessageListener<MessageBookmarkUpdate> {
public:
  using Ptr = std::shared_ptr<Application>;

  /**
   * @brief Creates the singleton instance of the Application.
   * @param version The version of the application.
   * @param factory Base factory object.
   * @param viewFactory Pointer to the ViewFactory.
   * @param networkFactory Pointer to the NetworkFactory.
   */
  static void createInstance(const Version& version,
                             std::shared_ptr<lib::IFactory> factory,
                             ViewFactory* viewFactory,
                             NetworkFactory* networkFactory);

  /**
   * @brief Gets the singleton instance of the Application.
   * @return Shared pointer to the Application instance.
   */
  static Ptr getInstance() {
    // TODO(Hussein): It breaks a lot of tests
    // assert(sInstance);
    return sInstance;
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
  static void loadStyle(const std::filesystem::path& colorSchemePath);

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
  [[nodiscard]] std::shared_ptr<IProject> getCurrentProject() const noexcept {
    return mProject;
  }

  /**
   * @brief Gets the path of the current project.
   * @return FilePath of the current project.
   */
  [[nodiscard]] FilePath getCurrentProjectPath() const noexcept {
    return mProject ? mProject->getProjectSettingsFilePath() : FilePath{};
  }

  /**
   * @brief Checks if a project is currently loaded.
   * @return True if a project is loaded, false otherwise.
   */
  [[nodiscard]] bool isProjectLoaded() const noexcept {
    return mProject && mProject->isLoaded();
  }

  /**
   * @brief Checks if the application has a GUI.
   * @return True if the application has a GUI, false otherwise.
   */
  [[nodiscard]] bool hasGUI() const noexcept {
    return mHasGui;
  }

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
  static std::shared_ptr<Application> sInstance;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static std::string sUuid;                         // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  Application(std::shared_ptr<lib::IFactory> factory, bool withGUI = true);

  /**
   * @name handle Message group
   * @{ */
  void handleMessage(MessageActivateWindow* pMessage) override;
  void handleMessage(MessageCloseProject* pMessage) override;
  void handleMessage(MessageIndexingFinished* pMessage) override;
  /**
   * @brief Handle a load project message
   *
   * @param message a message
   */
  void handleMessage(MessageLoadProject* message) override;
#if !defined(SOURCETRAIL_WASM)
  void handleMessage(MessageRefresh* pMessage) override;
#endif
  void handleMessage(MessageRefreshUI* pMessage) override;
  void handleMessage(MessageSwitchColorScheme* pMessage) override;

  void handleMessage(MessageBookmarkUpdate* message) override;
  /**  @} */

  void startMessagingAndScheduling();

  void loadWindow(bool showStartWindow);

#if !defined(SOURCETRAIL_WASM)
  void refreshProject(RefreshMode refreshMode, bool shallowIndexingRequested);
#endif
  void updateRecentProjects(const std::filesystem::path& projectSettingsFilePath);

  void logStorageStats() const;

  void updateTitle();

  bool checkSharedMemory();

  const bool mHasGui;
  std::shared_ptr<lib::IFactory> mFactory;
  bool mLoadedWindow = false;

  std::shared_ptr<IProject> mProject;
  std::shared_ptr<StorageCache> mStorageCache;

  std::shared_ptr<MainView> mMainView;

  std::shared_ptr<IDECommunicationController> mIdeCommunicationController;
};
