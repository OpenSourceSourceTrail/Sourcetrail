#include "HttpProject.h"

#include <chrono>

#include "app/Application.h"
#include "engine.pb.h"
#include "EngineCall.h"
#include "EngineChannel.h"
#include "logging.h"
#include "ProtoJson.h"
#include "settings/ProjectSettings.h"
#include "type/indexing/MessageIndexingFinished.h"
#include "type/MessageStatus.h"
#include "utilityString.h"

using client::call;

namespace {
// Loading opens the database and builds its caches; on a large project that is minutes, not the
// channel's UI-sized default deadline.
constexpr std::chrono::milliseconds LoadProjectTimeout{600000};
}    // namespace

HttpProject::HttpProject(EngineChannel* channel, std::shared_ptr<ProjectSettings> settings)
    : mChannel(channel), mSettings(std::move(settings)) {}

FilePath HttpProject::getProjectSettingsFilePath() const {
  return mSettings->getFilePath();
}

std::string HttpProject::getDescription() const {
  return mDescription;
}

bool HttpProject::isLoaded() const {
  return mLoaded;
}

bool HttpProject::isIndexing() const {
  return mIndexing;
}

bool HttpProject::isReindexable() const {
  return mReindexable;
}

bool HttpProject::settingsEqualExceptNameAndLocation(const ProjectSettings& otherSettings) const {
  // Purely a settings comparison; the engine has nothing to add and a round trip would only make
  // the project wizard slower.
  return mSettings->equalsExceptNameAndLocation(otherSettings);
}

void HttpProject::load(const std::shared_ptr<DialogView>& /*dialogView*/) {
  sourcetrail::LoadProjectRequest request;
  request.set_project_file_path(utility::encodeToUtf8(getProjectSettingsFilePath().wstr()));

  const auto response = call<sourcetrail::LoadProjectResponse>(
      mChannel, "loadProject", "PUT", "/api/v1/project", proto::json::toJson(request), LoadProjectTimeout);
  if(!response) {
    // The supervisor is already restarting the engine; say so rather than claiming the project is
    // broken, and leave the cached state alone so the views keep whatever they were showing.
    MessageStatus(L"Cannot load the project: the engine is not available.", true).dispatch();
    return;
  }

  mLoaded = response->loaded();
  mIndexing = response->indexing();
  mReindexable = response->reindexable();
  mDescription = response->description();

  if(!response->error_message().empty()) {
    MessageStatus(utility::decodeFromUtf8(response->error_message()), true).dispatch();
  }

  // Project::load does exactly this once the storage is readable; the client is where the GUI lives
  // now, so it is the one that has to say "the data is there, refresh everything". Without it no
  // view ever reloads and the project looks like it never opened.
  if(mLoaded && Application::getInstance()->hasGUI()) {
    MessageIndexingFinished{}.dispatch();
  }
}

void HttpProject::setStateOutdated() {
  std::ignore = client::callVoid(mChannel, "setStateOutdated", "POST", "/api/v1/project/state/outdated");
}

void HttpProject::refresh(std::shared_ptr<DialogView> /*dialogView*/, RefreshMode refreshMode, bool /*shallow*/) {
  sourcetrail::RefreshRequest request;
  request.set_all(refreshMode == RefreshMode::AllFiles);

  if(!client::callVoid(mChannel, "refresh", "POST", "/api/v1/project/refresh", proto::json::toJson(request))) {
    MessageStatus(L"Cannot refresh the project: the engine is not available.", true).dispatch();
    return;
  }
  mIndexing = true;
}

RefreshInfo HttpProject::getRefreshInfo(RefreshMode /*mode*/) const {
  // The refresh plan is computed engine-side from the database, so the client cannot derive it and
  // an empty one means "index nothing" -- the safe answer. Refreshing goes through
  // POST /api/v1/project/refresh, which lets the engine build and run the plan itself.
  return {};
}

void HttpProject::buildIndex(RefreshInfo /*info*/, std::shared_ptr<DialogView> /*dialogView*/) {
  LOG_WARNING("buildIndex is driven by the engine; the client only asks for a Refresh.");
}
