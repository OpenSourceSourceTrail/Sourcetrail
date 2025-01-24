#include "StorageCache.h"

#include "SourceLocationCollection.h"
#include "SourceLocationFile.h"
#include "TextAccess.h"
#include "utility.h"

void StorageCache::clear() {
  mGraphForAll.reset();
  mStorageStats = {};
  setUseErrorCache(false);
}

std::shared_ptr<Graph> StorageCache::getGraphForAll() const {
  if(!mGraphForAll) {
    mGraphForAll = StorageAccessProxy::getGraphForAll();
  }

  return mGraphForAll;
}

StorageStats StorageCache::getStorageStats() const {
  if(mStorageStats.nodeCount == 0U) {
    mStorageStats = StorageAccessProxy::getStorageStats();
  }

  return mStorageStats;
}

std::shared_ptr<TextAccess> StorageCache::getFileContent(const FilePath& filePath, bool showsErrors) const {
  if(mUseErrorCache && showsErrors) {
    return TextAccess::createFromFile(filePath);
  }

  return StorageAccessProxy::getFileContent(filePath, showsErrors);
}

ErrorCountInfo StorageCache::getErrorCount() const {
  if(!mUseErrorCache) {
    return StorageAccessProxy::getErrorCount();
  }

  return mErrorCount;
}

std::vector<ErrorInfo> StorageCache::getErrorsLimited(const ErrorFilter& filter) const {
  if(!mUseErrorCache) {
    return StorageAccessProxy::getErrorsLimited(filter);
  }

  return filter.filterErrors(mCachedErrors);
}

std::vector<ErrorInfo> StorageCache::getErrorsForFileLimited(const ErrorFilter& filter, const FilePath& filePath) const {
  if(!mUseErrorCache) {
    return StorageAccessProxy::getErrorsForFileLimited(filter, filePath);
  }

  return {};
}

std::shared_ptr<SourceLocationCollection> StorageCache::getErrorSourceLocations(const std::vector<ErrorInfo>& errors) const {
  std::shared_ptr<SourceLocationCollection> collection = StorageAccessProxy::getErrorSourceLocations(errors);

  if(mUseErrorCache) {
    std::map<std::wstring, bool> fileIndexed;
    for(const ErrorInfo& error : mCachedErrors) {
      fileIndexed.emplace(error.filePath, error.indexed);
    }

    collection->forEachSourceLocationFile([&](const std::shared_ptr<SourceLocationFile>& file) {
      file->setIsComplete(false);

      auto iterator = fileIndexed.find(file->getFilePath().wstr());
      if(iterator != fileIndexed.end()) {
        file->setIsIndexed(iterator->second);
      } else {
        file->setIsIndexed(true);
      }
    });
  }

  return collection;
}

void StorageCache::setUseErrorCache(bool enabled) {
  mUseErrorCache = enabled;
  mCachedErrors.clear();
  mErrorCount = {};
}

void StorageCache::addErrorsToCache(const std::vector<ErrorInfo>& newErrors, const ErrorCountInfo& errorCount) {
  utility::append(mCachedErrors, newErrors);
  mErrorCount = errorCount;
}
