#pragma once
#include <gmock/gmock.h>

#include "component/view/DialogView.h"
#include "data/indexer/IndexerCommand.h"
#include "data/indexer/IndexerCommandProvider.h"
#include "data/storage/StorageProvider.h"
#include "project/SourceGroup.h"
#include "settings/source_group/SourceGroupSettings.h"

struct MockedSourceGroup : SourceGroup {
  MOCK_METHOD(bool, prepareIndexing, (), (override));
  MOCK_METHOD(bool, allowsPartialClearing, (), (const, override));
  MOCK_METHOD(bool, allowsShallowIndexing, (), (const, override));

  MOCK_METHOD(std::set<FilePath>, filterToContainedFilePaths, (const std::set<FilePath>& filePaths), (const, override));
  MOCK_METHOD(std::set<FilePath>, getAllSourceFilePaths, (), (const, override));
  MOCK_METHOD(std::shared_ptr<IndexerCommandProvider>, getIndexerCommandProvider, (const RefreshInfo& info), (const, override));
  MOCK_METHOD(std::vector<std::shared_ptr<IndexerCommand>>, getIndexerCommands, (const RefreshInfo& info), (const, override));
  MOCK_METHOD(std::shared_ptr<Task>,
              getPreIndexTask,
              (std::shared_ptr<StorageProvider> storageProvider, std::shared_ptr<DialogView> dialogView),
              (const, override));

  MOCK_METHOD(std::shared_ptr<SourceGroupSettings>, getSourceGroupSettings, (), (override));
  MOCK_METHOD(std::shared_ptr<const SourceGroupSettings>, getSourceGroupSettings, (), (const, override));
};
