/**
 * @file StorageProvider.h
 * @author Malte Langkabel (mlangkabel@coati.io)
 * @brief A class that provides storages for the data processing
 * @version 0.1
 * @date 2025-01-10
 *
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <list>
#include <memory>
#include <mutex>

#include <nonstd/expected.hpp>

#include "IntermediateStorage.h"

/**
 * @brief A class that provides storages for the data processing
 */
class StorageProvider final {
public:
  /**
   * @brief Get the Storage Count object
   *
   * @note This function is thread-safe
   *
   * @return int The number of storages
   */
  [[nodiscard]] int getStorageCount() const noexcept;

  /**
   * @brief Clear all storages
   *
   * @note This function is thread-safe
   */
  void clear();

  /**
   * @brief Insert a storage into the provider
   *
   * @note This function is thread-safe
   *
   * @param storage The storage to insert
   * @return nonstd::expected<void, std::string> An error message if the storage is null
   */
  nonstd::expected<void, std::string> insert(std::shared_ptr<IntermediateStorage> storage) noexcept;

  /**
   * @brief Consume the second-largest storage
   *
   * @note This function is thread-safe
   *
   * @return std::shared_ptr<IntermediateStorage> The second-largest storage
   */
  nonstd::expected<std::shared_ptr<IntermediateStorage>, std::string> consumeSecondLargestStorage() noexcept;

  /**
   * @brief Consume the largest storage
   *
   * @note This function is thread-safe
   *
   * @return std::shared_ptr<IntermediateStorage> The largest storage
   */
  nonstd::expected<std::shared_ptr<IntermediateStorage>, std::string> consumeLargestStorage() noexcept;

private:
  std::list<std::shared_ptr<IntermediateStorage>> mStorages;    // larger storages are in front
  mutable std::mutex mStoragesMutex;
};
