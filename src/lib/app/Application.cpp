#include "Application.h"

#include <algorithm>
#include <filesystem>

#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "ColorScheme.h"
#include "CppSQLite3.h"
#include "DialogView.h"
#include "filter_types/MessageFilterErrorCountUpdate.h"
#include "filter_types/MessageFilterFocusInOut.h"
#include "filter_types/MessageFilterSearchAutocomplete.h"
#include "GraphViewStyle.h"
#include "IApplicationSettings.hpp"
#include "IDECommunicationController.h"
#include "impls/Factory.hpp"
#include "ISharedMemoryGarbageCollector.hpp"
#include "ITaskManager.hpp"
#include "logging.h"
#include "MainView.h"
#include "MessageQueue.h"
#include "NetworkFactory.h"
#include "ProjectSettings.h"
#include "SharedMemory.h"
#include "StorageCache.h"
#include "TabId.h"
#include "TaskScheduler.h"
#include "type/MessageQuitApplication.h"
#include "type/MessageStatus.h"
#include "UserPaths.h"
#include "utilityString.h"
#include "utilityUuid.h"
#include "Version.h"
#include "ViewFactory.h"


namespace fs = std::filesystem;

namespace {
std::wstring generateDatedFileName(const std::wstring& prefix, const std::wstring& suffix = L"", int offsetDays = 0) {
  time_t time{};
  std::time(&time);    // NOLINT

#ifdef _WIN32
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
  tm localTime = *std::localtime(&time);    // NOLINT

  if(0 != offsetDays) {
    time = mktime(&localTime) + offsetDays * 24 * 60 * 60;    // NOLINT
    localTime = *std::localtime(&time);                       // NOLINT
  }
#ifdef _WIN32
#  pragma warning(pop)
#endif

  std::wstringstream filename;
  if(!prefix.empty()) {
    filename << prefix << L"_";
  }

  filename << localTime.tm_year + 1900 << L"-";                                       // NOLINT
  filename << (localTime.tm_mon < 9 ? L"0" : L"") << localTime.tm_mon + 1 << L"-";    // NOLINT
  filename << (localTime.tm_mday < 10 ? L"0" : L"") << localTime.tm_mday << L"_";     // NOLINT
  filename << (localTime.tm_hour < 10 ? L"0" : L"") << localTime.tm_hour << L"-";     // NOLINT
  filename << (localTime.tm_min < 10 ? L"0" : L"") << localTime.tm_min << L"-";       // NOLINT
  filename << (localTime.tm_sec < 10 ? L"0" : L"") << localTime.tm_sec;               // NOLINT

  if(!suffix.empty()) {
    filename << L"_" << suffix;
  }

  return filename.str() + L".log";
}
void setupLogging(const std::string& logFileEnv) {
  try {
    std::vector<spdlog::sink_ptr> sinkList;
    sinkList.reserve(2);

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::info);
    sinkList.emplace_back(std::move(consoleSink));

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFileEnv, true);
    fileSink->set_level(spdlog::level::info);
    sinkList.emplace_back(std::move(fileSink));

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("multi_sink", sinkList.begin(), sinkList.end()));
  } catch(const spdlog::spdlog_ex& ex) {
    fmt::print(stderr, "{}\n", ex.what());
  }
}
}    // namespace

Application::Ptr Application::sInstance;
std::string Application::sUuid;

void Application::createInstance(const Version& version,
                                 std::shared_ptr<lib::IFactory> factory,
                                 ViewFactory* viewFactory,
                                 NetworkFactory* networkFactory) {
  const bool hasGui = (nullptr != viewFactory);

  Version::setApplicationVersion(version);

  if(hasGui) {
    GraphViewStyle::setImpl(viewFactory->createGraphStyleImpl());
  }

  // TODO(Hussein): We should create this iff multi-process is used
  if(auto collector = factory->createSharedMemoryGarbageCollector(); collector) {
    lib::ISharedMemoryGarbageCollector::setInstance(collector);
    collector->run(Application::getUUID());
  }

  scheduling::ITaskManager::setInstance(factory->createTaskManager());
  scheduling::ITaskManager::getInstanceRaw()->createScheduler(TabId::app());
  scheduling::ITaskManager::getInstanceRaw()->createScheduler(TabId::background());
  IMessageQueue::setInstance(factory->createMessageQueue());

  loadSettings();    // Must be called after creating IMessageQueue

  sInstance = std::shared_ptr<Application>(new Application(std::move(factory), hasGui));
  sInstance->mStorageCache = std::make_shared<StorageCache>();

  if(hasGui) {
    sInstance->mMainView = viewFactory->createMainView(sInstance->mStorageCache.get());
    sInstance->mMainView->setup();
  }

  if(nullptr != networkFactory) {
    sInstance->mIdeCommunicationController = networkFactory->createIDECommunicationController(sInstance->mStorageCache.get());
    sInstance->mIdeCommunicationController->startListening();
  }

  sInstance->startMessagingAndScheduling();
}

void Application::destroyInstance() {
  LOG_INFO("destroyInstance");
  IMessageQueue::getInstance()->stopMessageLoop();
  scheduling::ITaskManager::getInstanceRaw()->destroyScheduler(TabId::background());
  scheduling::ITaskManager::getInstanceRaw()->destroyScheduler(TabId::app());

  sInstance.reset();
}

std::string Application::getUUID() {
  if(sUuid.empty()) {
    sUuid = utility::getUuidString();
  }

  return sUuid;
}

void Application::loadSettings() {
  MessageStatus(fmt::format(L"Load settings: {}", UserPaths::getAppSettingsFilePath().wstr())).dispatch();

  auto* settings = IApplicationSettings::getInstanceRaw();
  if(auto settingsPath = UserPaths::getAppSettingsFilePath(); !settings->load(settingsPath)) {
    LOG_WARNING(fmt::format(L"Failed to load ApplicationSettings from the following path \"{}\"", settingsPath.wstr()));
  }

  if(settings->getLoggingEnabled()) {
    if(auto* dLogger = spdlog::default_logger_raw(); nullptr != dLogger) {
      namespace fs = std::filesystem;
      const auto loggerPath = (fs::path{settings->getLogDirectoryPath().wstring()} / generateDatedFileName(L"log")).string();
      if(auto& sinks = dLogger->sinks(); sinks.empty()) {
        setupLogging(loggerPath);
      } else {
        if(auto itr = std::ranges::find_if(
               sinks, [](const auto& sink) { return dynamic_cast<spdlog::sinks::basic_file_sink_mt*>(sink.get()) != nullptr; });
           sinks.end() != itr) {
          sinks.erase(itr);
        }

        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(loggerPath, true));
        const auto level = settings->getLoggingLevel();
        for(auto& sink : sinks) {
          sink->set_level(static_cast<spdlog::level::level_enum>(level));
        }
      }
    }
  }

  loadStyle(settings->getColorSchemePath());
}

void Application::loadStyle(const fs::path& colorSchemePath) {
  if(!ColorScheme::getInstance()->load(FilePath{colorSchemePath.wstring()})) {
    LOG_WARNING(fmt::format(L"Failed to load Style from the following path \"{}\"", colorSchemePath.wstring()));
  }
  GraphViewStyle::loadStyleSettings();
}

Application::Application(std::shared_ptr<lib::IFactory> factory, bool withGUI) : mHasGui(withGUI), mFactory{std::move(factory)} {}

Application::~Application() {
  if(mHasGui) {
    mMainView->saveLayout();
  }

  if(auto* collector = lib::ISharedMemoryGarbageCollector::getInstanceRaw(); collector) {
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
  if(mMainView) {
    return mMainView->getDialogView(useCase);
  }

  return std::make_shared<DialogView>(useCase, nullptr);
}

void Application::updateHistoryMenu(std::shared_ptr<MessageBase> message) {
  if(!message) {
    LOG_INFO("The message is empty");
  }
  if(mMainView) {
    mMainView->updateHistoryMenu(std::move(message));
  }
}

void Application::handleMessage(MessageActivateWindow* /*pMessage*/) {
  if(mHasGui) {
    mMainView->activateWindow();
  }
}

void Application::handleMessage(MessageCloseProject* /*pMessage*/) {
  if(mProject && mProject->isIndexing()) {
    MessageStatus(L"Cannot close the project while indexing.", true, false).dispatch();
    return;
  }

  mProject.reset();
  updateTitle();
  mMainView->clear();
}

void Application::handleMessage(MessageIndexingFinished* /*pMessage*/) {
  logStorageStats();

  if(mHasGui) {
    MessageRefreshUI().afterIndexing().dispatch();
  } else {
    MessageQuitApplication().dispatch();
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Application::handleMessage(MessageLoadProject* message) {
  if(nullptr == message) {
    return;
  }

  const auto projectSettingsFilePath = message->projectSettingsFilePath;
  loadWindow(projectSettingsFilePath.empty());

  if(projectSettingsFilePath.empty()) {
    return;
  }

  if(mProject && mProject->isIndexing()) {
    MessageStatus(L"Cannot load another project while indexing.", true, false).dispatch();
    return;
  }

  if(mProject && mProject->getProjectSettingsFilePath() == projectSettingsFilePath) {
    if(message->settingsChanged && mHasGui) {
      mProject->setStateOutdated();
#if !defined(SOURCETRAIL_WASM)
      refreshProject(RefreshMode::AllFiles, message->shallowIndexingRequested);
#endif
    }
  } else {
    MessageStatus(L"Loading Project: " + projectSettingsFilePath.wstr(), false, true).dispatch();

    mProject.reset();

    if(mHasGui) {
      mMainView->clear();
    }

    try {
      updateRecentProjects(fs::path{projectSettingsFilePath.wstr()});

      mStorageCache->clear();
      // TODO: check if this is really required.
      mStorageCache->setSubject(std::weak_ptr<StorageAccess>());

      mProject = mFactory->createProject(
          std::make_shared<ProjectSettings>(projectSettingsFilePath), mStorageCache.get(), getUUID(), hasGUI());

      if(mProject) {
        mProject->load(getDialogView(DialogView::UseCase::GENERAL));
      } else {
        LOG_ERROR("Failed to load project.");
        MessageStatus(L"Failed to load project: " + projectSettingsFilePath.wstr(), true).dispatch();
      }

      updateTitle();
    } catch(CppSQLite3Exception& cppSQLite3Exception) {
      const auto errorMessage = fmt::format(L"Failed to load project at \"{}\" with sqlite exception: {}",
                                            projectSettingsFilePath.wstr(),
                                            utility::decodeFromUtf8(cppSQLite3Exception.errorMessage()));
      LOG_ERROR(errorMessage);
      MessageStatus(errorMessage, true).dispatch();
    } catch(std::exception& exception) {
      const auto errorMessage = fmt::format(L"Failed to load project at \"{}\" with exception: {}",
                                            projectSettingsFilePath.wstr(),
                                            utility::decodeFromUtf8(exception.what()));
      LOG_ERROR(errorMessage);
      MessageStatus(errorMessage, true).dispatch();
    } catch(...) {
      const auto errorMessage = fmt::format(
          L"Failed to load project at \"{}\" with unknown exception.", projectSettingsFilePath.wstr());
      LOG_ERROR(errorMessage);
      MessageStatus(errorMessage, true).dispatch();
    }

    if(RefreshMode::None != message->refreshMode) {
#if !defined(SOURCETRAIL_WASM)
      refreshProject(message->refreshMode, message->shallowIndexingRequested);
#endif
    }
  }
}

#if !defined(SOURCETRAIL_WASM)
void Application::handleMessage(MessageRefresh* pMessage) {
  refreshProject(pMessage->all ? RefreshMode::AllFiles : RefreshMode::UpdatedFiles, false);
}
#endif

void Application::handleMessage(MessageRefreshUI* pMessage) {
  if(mHasGui) {
    updateTitle();

    if(pMessage->loadStyle) {
      loadStyle(IApplicationSettings::getInstanceRaw()->getColorSchemePath());
    }

    mMainView->refreshViews();

    mMainView->refreshUIState(pMessage->isAfterIndexing);
  }
}

void Application::handleMessage(MessageSwitchColorScheme* pMessage) {
  MessageStatus(L"Switch color scheme: " + pMessage->colorSchemePath.wstr()).dispatch();

  loadStyle(pMessage->colorSchemePath.wstr());
  MessageRefreshUI().noStyleReload().dispatch();
}

void Application::handleMessage(MessageBookmarkUpdate* message) {
  assert(message != nullptr);
  if(!mMainView) {
    LOG_WARNING("MainView isn't initialized");
    return;
  }
  mMainView->updateBookmarksMenu(message->mBookmarks);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Application::startMessagingAndScheduling() {
  scheduling::ITaskManager::getInstanceRaw()->getScheduler(TabId::app())->startSchedulerLoopThreaded();
  scheduling::ITaskManager::getInstanceRaw()->getScheduler(TabId::background())->startSchedulerLoopThreaded();

  IMessageQueue* queue = IMessageQueue::getInstance().get();
  queue->addMessageFilter(std::make_shared<MessageFilterErrorCountUpdate>());
  queue->addMessageFilter(std::make_shared<MessageFilterFocusInOut>());
  queue->addMessageFilter(std::make_shared<MessageFilterSearchAutocomplete>());

  queue->setSendMessagesAsTasks(true);
  queue->startMessageLoopThreaded();
}

void Application::loadWindow(bool showStartWindow) {
  if(!mHasGui) {
    return;
  }

  if(!mLoadedWindow) {
    [[maybe_unused]] IApplicationSettings* appSettings = IApplicationSettings::getInstanceRaw();

    updateTitle();

    mMainView->loadWindow(showStartWindow);
    mLoadedWindow = true;
  } else if(!showStartWindow) {
    mMainView->hideStartScreen();
  }
}

#if !defined(SOURCETRAIL_WASM)
void Application::refreshProject(RefreshMode refreshMode, bool shallowIndexingRequested) {
  if(mProject && checkSharedMemory()) {
    mProject->refresh(getDialogView(DialogView::UseCase::INDEXING), refreshMode, shallowIndexingRequested);

    if(!mHasGui && !mProject->isIndexing()) {
      MessageQuitApplication().dispatch();
    }
  }
}
#endif

void Application::updateRecentProjects(const fs::path& projectSettingsFilePath) {
  if(mHasGui) {
    auto* appSettings = IApplicationSettings::getInstanceRaw();
    std::vector<fs::path> recentProjects = appSettings->getRecentProjects();
    if(auto found = std::ranges::find(recentProjects, projectSettingsFilePath); found != recentProjects.end()) {
      recentProjects.erase(found);
    }

    recentProjects.insert(recentProjects.begin(), projectSettingsFilePath);
    while(recentProjects.size() > appSettings->getMaxRecentProjectsCount()) {
      recentProjects.pop_back();
    }

    appSettings->setRecentProjects(recentProjects);
    appSettings->save(UserPaths::getAppSettingsFilePath());

    mMainView->updateRecentProjectMenu();
  }
}

void Application::logStorageStats() const {
  if(!IApplicationSettings::getInstanceRaw()->getLoggingEnabled()) {
    return;
  }

  const StorageStats stats = mStorageCache->getStorageStats();
  const ErrorCountInfo errorCount = mStorageCache->getErrorCount();

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
  if(mHasGui) {
    std::wstring title = L"Sourcetrail";

    if(mProject) {
      const FilePath projectPath = mProject->getProjectSettingsFilePath();

      if(!projectPath.empty()) {
        title += L" - " + projectPath.fileName();
      }
    }

    mMainView->setTitle(title);
  }
}

bool Application::checkSharedMemory() {
  const std::wstring error = utility::decodeFromUtf8(SharedMemory::checkSharedMemory(getUUID()));
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
