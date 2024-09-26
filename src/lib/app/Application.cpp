#include "Application.h"

#include <algorithm>
#include <filesystem>

#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

// internal
#include "ColorScheme.h"
#include "CppSQLite3.h"
#include "DialogView.h"
#include "GraphViewStyle.h"
#include "IDECommunicationController.h"
#include "MainView.h"
#include "MessageFilterErrorCountUpdate.h"
#include "MessageFilterFocusInOut.h"
#include "MessageFilterSearchAutocomplete.h"
#include "MessageQueue.h"
#include "MessageQuitApplication.h"
#include "MessageStatus.h"
#include "NetworkFactory.h"
#include "ProjectSettings.h"
#include "SharedMemoryGarbageCollector.h"
#include "StorageCache.h"
#include "TabId.h"
#include "TaskManager.h"
#include "TaskScheduler.h"
#include "UserPaths.h"
#include "Version.h"
#include "ViewFactory.h"
#include "IApplicationSettings.hpp"
#include "logging.h"
#include "tracing.h"
#include "utilityString.h"
#include "utilityUuid.h"

namespace fs = std::filesystem;

namespace {
std::wstring generateDatedFileName(const std::wstring& prefix, const std::wstring& suffix = L"", int offsetDays = 0) {
  time_t time;
  std::time(&time);

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
  tm t = *std::localtime(&time);

  if(0 != offsetDays) {
    time = mktime(&t) + offsetDays * 24 * 60 * 60;
    t = *std::localtime(&time);
  }
#ifdef _WIN32
#pragma warning(pop)
#endif

  std::wstringstream filename;
  if(!prefix.empty()) {
    filename << prefix << L"_";
  }

  filename << t.tm_year + 1900 << L"-";
  filename << (t.tm_mon < 9 ? L"0" : L"") << t.tm_mon + 1 << L"-";
  filename << (t.tm_mday < 10 ? L"0" : L"") << t.tm_mday << L"_";
  filename << (t.tm_hour < 10 ? L"0" : L"") << t.tm_hour << L"-";
  filename << (t.tm_min < 10 ? L"0" : L"") << t.tm_min << L"-";
  filename << (t.tm_sec < 10 ? L"0" : L"") << t.tm_sec;

  if(!suffix.empty()) {
    filename << L"_" << suffix;
  }

  return filename.str();
}
}    // namespace

Application::Ptr Application::sInstance;
std::string Application::s_uuid;

void Application::createInstance(const Version& version, ViewFactory* viewFactory, NetworkFactory* networkFactory) {
  const bool hasGui = (nullptr != viewFactory);

  Version::setApplicationVersion(version);

  if(hasGui) {
    GraphViewStyle::setImpl(viewFactory->createGraphStyleImpl());
  }

  loadSettings();

  SharedMemoryGarbageCollector* collector = SharedMemoryGarbageCollector::createInstance();
  if(nullptr != collector) {
    collector->run(Application::getUUID());
  }

  TaskManager::createScheduler(TabId::app());
  TaskManager::createScheduler(TabId::background());
  MessageQueue::getInstance();

  sInstance = std::shared_ptr<Application>(new Application(hasGui));

  sInstance->m_storageCache = std::make_shared<StorageCache>();

  if(hasGui) {
    sInstance->m_mainView = viewFactory->createMainView(sInstance->m_storageCache.get());
    sInstance->m_mainView->setup();
  }

  if(networkFactory != nullptr) {
    sInstance->m_ideCommunicationController = networkFactory->createIDECommunicationController(sInstance->m_storageCache.get());
    sInstance->m_ideCommunicationController->startListening();
  }

  sInstance->startMessagingAndScheduling();
}

void Application::destroyInstance() {
  LOG_INFO("destroyInstance");
  MessageQueue::getInstance()->stopMessageLoop();
  TaskManager::destroyScheduler(TabId::background());
  TaskManager::destroyScheduler(TabId::app());

  sInstance.reset();
}

std::string Application::getUUID() {
  if(s_uuid.empty()) {
    s_uuid = utility::getUuidString();
  }

  return s_uuid;
}

void Application::loadSettings() {
  MessageStatus(fmt::format(L"Load settings: {}", UserPaths::getAppSettingsFilePath().wstr())).dispatch();

  auto settings = IApplicationSettings::getInstanceRaw();
  if(auto settingsPath = UserPaths::getAppSettingsFilePath(); !settings->load(settingsPath)) {
    LOG_WARNING_W(fmt::format(L"Failed to load ApplicationSettings from the following path \"{}\"", settingsPath.wstr()));
  }

  if(settings->getLoggingEnabled()) {
    namespace fs = std::filesystem;
    auto loggerPath = fs::path{settings->getLogDirectoryPath().wstring()} / generateDatedFileName(L"log");
    auto dLogger = spdlog::default_logger_raw();
    if(nullptr == dLogger) {
      return;
    }

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(loggerPath, true);
    fileSink->set_level(spdlog::level::trace);
    dLogger->sinks().push_back(std::move(fileSink));
  }

  loadStyle(settings->getColorSchemePath());
}

void Application::loadStyle(const fs::path& colorSchemePath) {
  ColorScheme::getInstance()->load(FilePath{colorSchemePath.wstring()});
  GraphViewStyle::loadStyleSettings();
}

Application::Application(bool withGUI) : m_hasGUI(withGUI) {}

Application::~Application() {
  if(m_hasGUI) {
    m_mainView->saveLayout();
  }

  if(auto* collector = SharedMemoryGarbageCollector::getInstance(); nullptr != collector) {
    collector->stop();
  }
}

int Application::handleDialog(const std::wstring& message) {
  return getDialogView(DialogView::UseCase::GENERAL)->confirm(message);
}

int Application::handleDialog(const std::wstring& message, const std::vector<std::wstring>& options) {
  return getDialogView(DialogView::UseCase::GENERAL)->confirm(message, options);
}

std::shared_ptr<DialogView> Application::getDialogView(DialogView::UseCase useCase) {
  if(m_mainView) {
    return m_mainView->getDialogView(useCase);
  }

  return std::make_shared<DialogView>(useCase, nullptr);
}

void Application::updateHistoryMenu(std::shared_ptr<MessageBase> message) {
  if(!message) {
    LOG_INFO("The message is empty");
  }
  m_mainView->updateHistoryMenu(std::move(message));
}

void Application::handleMessage(MessageActivateWindow* /*pMessage*/) {
  if(m_hasGUI) {
    m_mainView->activateWindow();
  }
}

void Application::handleMessage(MessageCloseProject* /*pMessage*/) {
  if(m_pProject && m_pProject->isIndexing()) {
    MessageStatus(L"Cannot close the project while indexing.", true, false).dispatch();
    return;
  }

  m_pProject.reset();
  updateTitle();
  m_mainView->clear();
}

void Application::handleMessage(MessageIndexingFinished* /*pMessage*/) {
  logStorageStats();

  if(m_hasGUI) {
    MessageRefreshUI().afterIndexing().dispatch();
  } else {
    MessageQuitApplication().dispatch();
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Application::handleMessage(MessageLoadProject* pMessage) {
  TRACE("app load project");

  const auto projectSettingsFilePath = pMessage->projectSettingsFilePath;
  loadWindow(projectSettingsFilePath.empty());

  if(projectSettingsFilePath.empty()) {
    return;
  }

  if(m_pProject && m_pProject->isIndexing()) {
    MessageStatus(L"Cannot load another project while indexing.", true, false).dispatch();
    return;
  }

  if(m_pProject && projectSettingsFilePath == m_pProject->getProjectSettingsFilePath()) {
    if(pMessage->settingsChanged && m_hasGUI) {
      m_pProject->setStateOutdated();
      refreshProject(REFRESH_ALL_FILES, pMessage->shallowIndexingRequested);
    }
  } else {
    MessageStatus(L"Loading Project: " + projectSettingsFilePath.wstr(), false, true).dispatch();

    m_pProject.reset();

    if(m_hasGUI) {
      m_mainView->clear();
    }

    try {
      updateRecentProjects(fs::path{projectSettingsFilePath.wstr()});

      m_storageCache->clear();
      // TODO: check if this is really required.
      m_storageCache->setSubject(std::weak_ptr<StorageAccess>());

      m_pProject = std::make_shared<Project>(
          std::make_shared<ProjectSettings>(projectSettingsFilePath), m_storageCache.get(), getUUID(), hasGUI());

      if(m_pProject) {
        m_pProject->load(getDialogView(DialogView::UseCase::GENERAL));
      } else {
        LOG_ERROR("Failed to load project.");
        MessageStatus(L"Failed to load project: " + projectSettingsFilePath.wstr(), true).dispatch();
      }

      updateTitle();
    } catch(std::exception& e) {
      const std::wstring message = L"Failed to load project at \"" + projectSettingsFilePath.wstr() + L"\" with exception: " +
          utility::decodeFromUtf8(e.what());
      LOG_ERROR_W(message);
      MessageStatus(message, true).dispatch();
    } catch(CppSQLite3Exception& e) {
      const std::wstring message = L"Failed to load project at \"" + projectSettingsFilePath.wstr() +
          L"\" with sqlite exception: " + utility::decodeFromUtf8(e.errorMessage());
      LOG_ERROR_W(message);
      MessageStatus(message, true).dispatch();
    } catch(...) {
      const std::wstring message = L"Failed to load project at \"" + projectSettingsFilePath.wstr() +
          L"\" with unknown exception.";
      LOG_ERROR_W(message);
      MessageStatus(message, true).dispatch();
    }

    if(pMessage->refreshMode != REFRESH_NONE) {
      refreshProject(pMessage->refreshMode, pMessage->shallowIndexingRequested);
    }
  }
}

void Application::handleMessage(MessageRefresh* pMessage) {
  TRACE("app refresh");

  refreshProject(pMessage->all ? REFRESH_ALL_FILES : REFRESH_UPDATED_FILES, false);
}

void Application::handleMessage(MessageRefreshUI* pMessage) {
  TRACE("ui refresh");

  if(m_hasGUI) {
    updateTitle();

    if(pMessage->loadStyle) {
      loadStyle(IApplicationSettings::getInstanceRaw()->getColorSchemePath());
    }

    m_mainView->refreshViews();

    m_mainView->refreshUIState(pMessage->isAfterIndexing);
  }
}

void Application::handleMessage(MessageSwitchColorScheme* pMessage) {
  MessageStatus(L"Switch color scheme: " + pMessage->colorSchemePath.wstr()).dispatch();

  loadStyle(pMessage->colorSchemePath.wstr());
  MessageRefreshUI().noStyleReload().dispatch();
}

void Application::handleMessage(MessageBookmarkUpdate* message) {
  assert(message != nullptr);
  if(!m_mainView) {
    LOG_WARNING("MainView isn't initialized");
    return;
  }
  m_mainView->updateBookmarksMenu(message->mBookmarks);
}

void Application::startMessagingAndScheduling() {
  TaskManager::getScheduler(TabId::app())->startSchedulerLoopThreaded();
  TaskManager::getScheduler(TabId::background())->startSchedulerLoopThreaded();

  MessageQueue* queue = MessageQueue::getInstance().get();
  queue->addMessageFilter(std::make_shared<MessageFilterErrorCountUpdate>());
  queue->addMessageFilter(std::make_shared<MessageFilterFocusInOut>());
  queue->addMessageFilter(std::make_shared<MessageFilterSearchAutocomplete>());

  queue->setSendMessagesAsTasks(true);
  queue->startMessageLoopThreaded();
}

void Application::loadWindow(bool showStartWindow) {
  if(!m_hasGUI) {
    return;
  }

  if(!m_loadedWindow) {
    [[maybe_unused]] IApplicationSettings* appSettings = IApplicationSettings::getInstanceRaw();

    updateTitle();

    m_mainView->loadWindow(showStartWindow);
    m_loadedWindow = true;
  } else if(!showStartWindow) {
    m_mainView->hideStartScreen();
  }
}

void Application::refreshProject(RefreshMode refreshMode, bool shallowIndexingRequested) {
  if(m_pProject && checkSharedMemory()) {
    m_pProject->refresh(getDialogView(DialogView::UseCase::INDEXING), refreshMode, shallowIndexingRequested);

    if(!m_hasGUI && !m_pProject->isIndexing()) {
      MessageQuitApplication().dispatch();
    }
  }
}

void Application::updateRecentProjects(const fs::path& projectSettingsFilePath) {
  if(m_hasGUI) {
    auto appSettings = IApplicationSettings::getInstanceRaw();
    std::vector<fs::path> recentProjects = appSettings->getRecentProjects();
    if(auto found = std::find(recentProjects.begin(), recentProjects.end(), projectSettingsFilePath);
       found != recentProjects.end()) {
      recentProjects.erase(found);
    }

    recentProjects.insert(recentProjects.begin(), projectSettingsFilePath);
    while(recentProjects.size() > appSettings->getMaxRecentProjectsCount()) {
      recentProjects.pop_back();
    }

    appSettings->setRecentProjects(recentProjects);
    appSettings->save(UserPaths::getAppSettingsFilePath());

    m_mainView->updateRecentProjectMenu();
  }
}

void Application::logStorageStats() const {
  if(!IApplicationSettings::getInstanceRaw()->getLoggingEnabled()) {
    return;
  }

  const StorageStats stats = m_storageCache->getStorageStats();
  const ErrorCountInfo errorCount = m_storageCache->getErrorCount();

  LOG_INFO(
      fmt::format("\nGraph:\n"
                  "\t{} Nodes\n"
                  "\t{} Edges\n"
                  "\nCode:\n"
                  "\t{} Files\n"
                  "\t{} Lines of Code\n"
                  "\nErrors:\n"
                  "\t{} Errors\n"
                  "\t{} Fatal Errors\n",
                  stats.nodeCount,
                  stats.edgeCount,
                  stats.fileCount,
                  stats.fileLOCCount,
                  errorCount.total,
                  errorCount.fatal));
}

void Application::updateTitle() {
  if(m_hasGUI) {
    std::wstring title = L"Sourcetrail";

    if(m_pProject) {
      FilePath projectPath = m_pProject->getProjectSettingsFilePath();

      if(!projectPath.empty()) {
        title += L" - " + projectPath.fileName();
      }
    }

    m_mainView->setTitle(title);
  }
}

bool Application::checkSharedMemory() {
  std::wstring error = utility::decodeFromUtf8(SharedMemory::checkSharedMemory(getUUID()));
  if(error.empty()) {
    return true;
  }
  MessageStatus(fmt::format(L"Error on accessing shared memory. Indexing not possible. "
                            "Please restart computer or run as admin: {}",
                            error),
                true)
      .dispatch();
  handleDialog(
      fmt::format(L"There was an error accessing shared memory on your computer: {}\n\n"
                  "Project indexing is not possible. Please restart your computer or try running "
                  "Sourcetrail as admin.",
                  error));
  return false;
}
