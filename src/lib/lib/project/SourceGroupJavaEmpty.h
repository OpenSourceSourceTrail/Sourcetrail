#pragma once

#include <memory>
#include <set>
#include <vector>

#include "SourceGroup.h"

class SourceGroupSettingsJavaEmpty;

class SourceGroupJavaEmpty : public SourceGroup {
public:
  SourceGroupJavaEmpty(std::shared_ptr<SourceGroupSettingsJavaEmpty> settings);

  [[nodiscard]] std::set<FilePath> filterToContainedFilePaths(const std::set<FilePath>& filePaths) const override;
  [[nodiscard]] std::set<FilePath> getAllSourceFilePaths() const override;
  [[nodiscard]] std::shared_ptr<IndexerCommandProvider> getIndexerCommandProvider(const RefreshInfo& info) const override;
  [[nodiscard]] std::vector<std::shared_ptr<IndexerCommand>> getIndexerCommands(const RefreshInfo& info) const override;

private:
  std::shared_ptr<SourceGroupSettings> getSourceGroupSettings() override;
  std::shared_ptr<const SourceGroupSettings> getSourceGroupSettings() const override;

  std::shared_ptr<SourceGroupSettingsJavaEmpty> m_settings;
};
