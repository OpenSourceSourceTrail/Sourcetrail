#pragma once
#include <gmock/gmock.h>

#include "IFileSystem.hpp"

namespace core::utility::filesystem {

class MockedFileSystem : public IFileSystem {
public:
  ~MockedFileSystem() noexcept override;

  MOCK_METHOD(Result<>, create_directory, (const std::filesystem::path&), (const, noexcept, override));

  MOCK_METHOD(Result<>, create_directory, (const std::filesystem::path&, const std::filesystem::path&), (const, noexcept, override));

  MOCK_METHOD(Result<>, create_directories, (const std::filesystem::path&), (const, noexcept, override));

  MOCK_METHOD(Result<std::filesystem::path>, current_path, (), (const, noexcept, override));

  MOCK_METHOD(Result<>, current_path, (const std::filesystem::path&), (const, noexcept, override));

  MOCK_METHOD(Result<>, exists, (const std::filesystem::path&), (const, noexcept, override));

  MOCK_METHOD(Result<std::uintmax_t>, file_size, (const std::filesystem::path&), (const, noexcept, override));

  MOCK_METHOD(Result<>,
              permissions,
              (const std::filesystem::path&, std::filesystem::perms, std::filesystem::perm_options),
              (const, noexcept, override));

  MOCK_METHOD(Result<>, remove, (const std::filesystem::path& path), (const, noexcept, override));

  MOCK_METHOD(Result<std::uintmax_t>, remove_all, (const std::filesystem::path&), (const, noexcept, override));

  MOCK_METHOD(Result<>, rename, (const std::filesystem::path&, const std::filesystem::path&), (const, noexcept, override));
};

}    // namespace core::utility::filesystem
