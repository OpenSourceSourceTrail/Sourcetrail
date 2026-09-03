#pragma once
#include <string>

#include "project/logic/IProject.hpp"

class EngineChannel;

/**
 * IProject implemented over the engine, for a client that owns no storage of its own.
 *
 * Everything that describes the project -- loaded, indexing, reindexable, description -- is a cached
 * copy of the last LoadProjectResponse, because Qt calls these from enable/paint paths where a round
 * trip would stall the UI. Commands do go to the engine, and a failed one leaves the cached state
 * untouched rather than throwing.
 */
class HttpProject final : public IProject {
public:
  HttpProject(EngineChannel* channel, std::shared_ptr<ProjectSettings> settings);

  [[nodiscard]] FilePath getProjectSettingsFilePath() const override;
  [[nodiscard]] std::string getDescription() const override;
  [[nodiscard]] bool isLoaded() const override;
  [[nodiscard]] bool isIndexing() const override;
  [[nodiscard]] bool isReindexable() const override;
  [[nodiscard]] bool settingsEqualExceptNameAndLocation(const ProjectSettings& otherSettings) const override;

  void setStateOutdated() override;
  void load(const std::shared_ptr<DialogView>& dialogView) override;
  void refresh(std::shared_ptr<DialogView> dialogView, RefreshMode refreshMode, bool shallowIndexingRequested) override;
  [[nodiscard]] RefreshInfo getRefreshInfo(RefreshMode mode) const override;
  void buildIndex(RefreshInfo info, std::shared_ptr<DialogView> dialogView) override;

private:
  EngineChannel* mChannel;
  std::shared_ptr<ProjectSettings> mSettings;

  std::string mDescription;
  bool mLoaded = false;
  bool mIndexing = false;
  // Assumed true until the engine says otherwise: the alternative greys out Refresh for every
  // project the moment the engine is slow to answer.
  bool mReindexable = true;
};
