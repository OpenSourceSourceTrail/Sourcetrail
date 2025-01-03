/**
 * @file IFileSystem.hpp
 * @author Ahmed Abdel Aal (eng.ahmedhussein@gmail.com)
 * @brief Interface for file system operations.
 * @date 2025-01-01
 *
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <filesystem>
#include <memory>

#include <nonstd/expected.hpp>

namespace core::utility::filesystem {

template <typename T = void>
using Result = nonstd::expected<T, std::string>;

/**
 * @brief Interface for file system operations.
 */
struct IFileSystem {
  using Raw = IFileSystem*;
  using Ptr = std::shared_ptr<IFileSystem>;

  /**
   * @brief Get the instance object
   *
   * @return Ptr the instance object
   */
  static Ptr instance() noexcept;

  /**
   * @brief Get the Instance Raw object
   *
   * @return Raw the instance raw object
   */
  static Raw getInstanceRaw() noexcept;

  /**
   * @brief Set the instance object
   *
   * @param instance the instance object
   */
  static void setInstance(Ptr instance) noexcept;

  virtual ~IFileSystem() noexcept;

  /**
   * @brief Create a directory at the given path.
   *
   * @param path Path to the directory.
   * @return Result true if the directory was created, error as string otherwise.
   */
  [[nodiscard]] virtual Result<> create_directory(const std::filesystem::path& path) const noexcept = 0;

  /**
   * @brief Create a directory at the given path.
   *
   * @param path Path to the directory.
   * @param existingPath Path to the existing directory.
   * @return Result true if the directory was created, error as string otherwise.
   */
  [[nodiscard]] virtual Result<> create_directory(const std::filesystem::path& path,
                                                  const std::filesystem::path& existingPath) const noexcept = 0;

  /**
   * @brief Create a directories at the given path.
   *
   * @param path Path to the directory.
   * @return Result true if the directory was created, error as string otherwise.
   */
  [[nodiscard]] virtual Result<> create_directories(const std::filesystem::path& path) const noexcept = 0;

  /**
   * @brief Get the absolute path of the current working directory.
   * @return Result the absolute current path if successful, error as string otherwise.
   */
  [[nodiscard]] virtual Result<std::filesystem::path> current_path() const noexcept = 0;

  /**
   * @brief Set the current working directory.
   * @return Result true if the file exists, error as string otherwise.
   */
  [[nodiscard]] virtual Result<> current_path(const std::filesystem::path& path) const noexcept = 0;

  /**
   * @brief Check if a file exists at the given path.
   *
   * @param path Path to the file.
   * @return Result true if the file exists, error as string otherwise.
   */
  [[nodiscard]] virtual Result<> exists(const std::filesystem::path& path) const noexcept = 0;

  /**
   * @brief Get the size of the file at the given path.
   *
   * @param path Path to the file.
   * @return Result<std::uintmax_t>  the size of the file if successful, error as string otherwise.
   */
  [[nodiscard]] virtual Result<std::uintmax_t> file_size(const std::filesystem::path& path) const noexcept = 0;

  /**
   * @brief Set the permissions of the file at the given path.
   *
   * @param path Path to the file.
   * @return Result empty in success Error as string otherwise.
   */
  [[nodiscard]] virtual Result<> permissions(
      const std::filesystem::path& path,
      std::filesystem::perms prms,
      std::filesystem::perm_options opts = std::filesystem::perm_options::replace) const noexcept = 0;

  /**
   * @brief Remove the file or directory at the given path.
   *
   * @param path Path to the file or directory.
   * @return Result<> empty in success Error as string otherwise.
   */
  [[nodiscard]] virtual Result<> remove(const std::filesystem::path& path) const noexcept = 0;

  /**
   * @brief Remove all files and directories at the given path.
   *
   * @param path Path to the file or directory.
   * @return Result<std::uintmax_t> the number of files and directories removed if successful, error as string otherwise.
   */
  [[nodiscard]] virtual Result<std::uintmax_t> remove_all(const std::filesystem::path& path) const noexcept = 0;

  /**
   * @brief Move or rename the file or directory at the given path.
   *
   * @param oldPath Path to the file or directory to move or rename.
   * @param newPath Non-exisiting Path to the file or directory.
   * @return Result<> empty in success Error as string otherwise.
   */
  [[nodiscard]] virtual Result<> rename(const std::filesystem::path& oldPath,
                                        const std::filesystem::path& newPath) const noexcept = 0;

private:
  static Ptr sInstance;
};

}    // namespace core::utility::filesystem
