#pragma once
#include <gmock/gmock.h>

#include "StorageCache.h"

struct MockedStorageCache : StorageCache {
  using GraphPtr = std::shared_ptr<Graph>;

  MOCK_METHOD(GraphPtr, getGraphForAll, (), (const, override));

  MOCK_METHOD(StorageStats, getStorageStats, (), (const, override));

  using TextAccessPtr = std::shared_ptr<TextAccess>;
  MOCK_METHOD(TextAccessPtr, getFileContent, (const FilePath&, bool), (const, override));

  MOCK_METHOD(ErrorCountInfo, getErrorCount, (), (const, override));

  using ErrorInfoVec = std::vector<ErrorInfo>;
  MOCK_METHOD(ErrorInfoVec, getErrorsLimited, (const ErrorFilter&), (const, override));

  MOCK_METHOD(ErrorInfoVec, getErrorsForFileLimited, (const ErrorFilter& filter, const FilePath& filePath), (const, override));

  using SourceLocationCollectionPtr = std::shared_ptr<SourceLocationCollection>;
  MOCK_METHOD(SourceLocationCollectionPtr, getErrorSourceLocations, (const ErrorInfoVec&), (const, override));

  MOCK_METHOD(void, setUseErrorCache, (bool), (override));
  MOCK_METHOD(void, addErrorsToCache, (const ErrorInfoVec&, const ErrorCountInfo&), (override));
};
