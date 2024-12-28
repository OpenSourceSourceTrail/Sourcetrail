#include "FileSystem.hpp"

#include <fmt/format.h>

#include "IFileSystem.hpp"

namespace core::utility::filesystem {

FileSystem::~FileSystem() noexcept = default;

Result<> FileSystem::create_directory(const std::filesystem::path& path) const noexcept {
  if(std::error_code errorCode; !std::filesystem::create_directory(path, errorCode)) {
    return nonstd::make_unexpected(fmt::format("Failed to create directory: {}", errorCode.message()));
  }
  return {};
}

Result<> FileSystem::create_directory(const std::filesystem::path& path, const std::filesystem::path& existingPath) const noexcept {
  if(std::error_code errorCode; !std::filesystem::create_directory(path, existingPath, errorCode)) {
    return nonstd::make_unexpected(fmt::format("Failed to create directory: {}", errorCode.message()));
  }
  return {};
}

Result<> FileSystem::create_directories(const std::filesystem::path& path) const noexcept {
  if(std::error_code errorCode; !std::filesystem::create_directories(path, errorCode)) {
    return nonstd::make_unexpected(fmt::format("Failed to create directories: {}", errorCode.message()));
  }
  return {};
}

Result<std::filesystem::path> FileSystem::current_path() const noexcept {
  std::error_code errorCode;
  if(auto currentPath = std::filesystem::current_path(errorCode); !errorCode) {
    return currentPath;
  }
  return nonstd::make_unexpected(fmt::format("Failed to get current path: {}", errorCode.message()));
}

Result<> FileSystem::current_path(const std::filesystem::path& path) const noexcept {
  std::error_code errorCode;
  std::filesystem::current_path(path, errorCode);
  if(!errorCode) {
    return {};
  }
  return nonstd::make_unexpected(fmt::format("Failed to set current path: {}", errorCode.message()));
}

Result<> FileSystem::exists(const std::filesystem::path& path) const noexcept {
  if(std::error_code errorCode; !std::filesystem::exists(path, errorCode)) {
    return nonstd::make_unexpected(fmt::format("File does not exist: {}", errorCode.message()));
  }
  return {};
}

Result<std::uintmax_t> FileSystem::file_size(const std::filesystem::path& path) const noexcept {
  std::error_code errorCode;
  if(const auto fileSize = std::filesystem::file_size(path, errorCode); !errorCode) {
    return fileSize;
  }
  return nonstd::make_unexpected(fmt::format("Failed to get file size: {}", errorCode.message()));
}

Result<> FileSystem::permissions(const std::filesystem::path& path,
                                 std::filesystem::perms prms,
                                 std::filesystem::perm_options opts) const noexcept {
  std::error_code errorCode;
  if(std::filesystem::permissions(path, prms, opts, errorCode); !errorCode) {
    return {};
  }
  return nonstd::make_unexpected(fmt::format("Failed to set file permission: {}", errorCode.message()));
}

Result<> FileSystem::remove(const std::filesystem::path& path) const noexcept {
  if(std::error_code errorCode; !std::filesystem::remove(path, errorCode)) {
    return nonstd::make_unexpected(fmt::format("Failed to remove file: {}", errorCode.message()));
  }
  return {};
}

Result<std::uintmax_t> FileSystem::remove_all(const std::filesystem::path& path) const noexcept {
  std::error_code errorCode;
  if(const auto removed = std::filesystem::remove_all(path, errorCode); !errorCode) {
    return removed;
  }
  return nonstd::make_unexpected(fmt::format("Failed to remove file: {}", errorCode.message()));
}

Result<> FileSystem::rename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath) const noexcept {
  std::error_code errorCode;
  if(std::filesystem::rename(oldPath, newPath, errorCode); errorCode) {
    return nonstd::make_unexpected(fmt::format("Failed to rename file: {}", errorCode.message()));
  }
  return {};
}

}    // namespace core::utility::filesystem
