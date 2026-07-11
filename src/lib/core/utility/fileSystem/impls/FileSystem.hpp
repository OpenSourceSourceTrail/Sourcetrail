#pragma once
#include "IFileSystem.hpp"

namespace core::utility::filesystem {

class FileSystem : public IFileSystem {
public:
  ~FileSystem() noexcept override;

  [[nodiscard]] Result<> create_directory(const std::filesystem::path& path) const noexcept override;

  [[nodiscard]] Result<> create_directory(const std::filesystem::path& path,
                                          const std::filesystem::path& existingPath) const noexcept override;

  [[nodiscard]] Result<> create_directories(const std::filesystem::path& path) const noexcept override;

  [[nodiscard]] Result<std::filesystem::path> current_path() const noexcept override;

  [[nodiscard]] Result<> current_path(const std::filesystem::path& path) const noexcept override;

  [[nodiscard]] Result<> exists(const std::filesystem::path& path) const noexcept override;

  [[nodiscard]] Result<std::uintmax_t> file_size(const std::filesystem::path& path) const noexcept override;

  [[nodiscard]] Result<> permissions(const std::filesystem::path& path,
                                     std::filesystem::perms prms,
                                     std::filesystem::perm_options opts = std::filesystem::perm_options::replace) const noexcept override;

  [[nodiscard]] Result<> remove(const std::filesystem::path& path) const noexcept override;

  [[nodiscard]] Result<std::uintmax_t> remove_all(const std::filesystem::path& path) const noexcept override;

  [[nodiscard]] Result<> rename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath) const noexcept override;
};

}    // namespace core::utility::filesystem
