/**
 * @file StorageCache.h
 * @author Eberhard Graether (egraether@coati.io)
 * @brief
 * @version 0.1
 * @date 2025-01-16
 *
 * @copyright Copyright (c) 2025
 */
#pragma once

#include "StorageAccessProxy.h"

/**
 * @brief A class that acts as a cache layer for storage access, inheriting from StorageAccessProxy.
 */
class StorageCache : public StorageAccessProxy {
public:
  /**
   * @brief Clear stored data.
   */
  void clear();

  /**
   * @brief Get the Graph For All object
   *
   * @return std::shared_ptr<Graph> The graph for all.
   */
  [[nodiscard]] std::shared_ptr<Graph> getGraphForAll() const override;

  /**
   * @brief Get the Storage Stats object
   *
   * @return StorageStats The storage stats.
   */
  [[nodiscard]] StorageStats getStorageStats() const override;

  /**
   * @brief Get the File Content object
   *
   * @param filePath The file path.
   * @param showsErrors Whether to show errors.
   * @return std::shared_ptr<TextAccess> The file content.
   */
  [[nodiscard]] std::shared_ptr<TextAccess> getFileContent(const FilePath& filePath, bool showsErrors) const override;

  /**
   * @brief Get the Error Count object
   *
   * @return ErrorCountInfo The error count.
   */
  [[nodiscard]] ErrorCountInfo getErrorCount() const override;

  /**
   * @brief Get the Errors Limited object
   *
   * @param filter The error filter.
   * @return std::vector<ErrorInfo> The errors.
   */
  [[nodiscard]] std::vector<ErrorInfo> getErrorsLimited(const ErrorFilter& filter) const override;

  /**
   * @brief Get the Errors For File Limited object
   *
   * @param filter The error filter.
   * @param filePath The file path.
   * @return std::vector<ErrorInfo> The errors.
   */
  [[nodiscard]] std::vector<ErrorInfo> getErrorsForFileLimited(const ErrorFilter& filter, const FilePath& filePath) const override;

  /**
   * @brief Get the Error Source Locations object
   *
   * @param errors The errors.
   * @return std::shared_ptr<SourceLocationCollection> The source location collection.
   */
  [[nodiscard]] std::shared_ptr<SourceLocationCollection> getErrorSourceLocations(const std::vector<ErrorInfo>& errors) const override;

  /**
   * @brief Set whether to use the error cache.
   *
   * @param enabled Whether to use the error cache.
   */
  void setUseErrorCache(bool enabled) override;

  /**
   * @brief Add errors to the cache.
   *
   * @param newErrors The new errors.
   * @param errorCount The error count.
   */
  void addErrorsToCache(const std::vector<ErrorInfo>& newErrors, const ErrorCountInfo& errorCount) override;

private:
  mutable std::shared_ptr<Graph> mGraphForAll;
  mutable StorageStats mStorageStats;

  bool mUseErrorCache = false;
  ErrorCountInfo mErrorCount;
  std::vector<ErrorInfo> mCachedErrors;
};
