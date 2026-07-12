#include "FileHandler.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>

#include <system_error>

FileHandler::Ptr FileHandler::createEmptyFile(fs::path filepath) {
  std::error_code errorCode;
  if(fs::exists(filepath, errorCode)) {
    // Logging
    return nullptr;
  }

  std::ofstream oStream(filepath);
  oStream.close();

  if(!fs::exists(filepath, errorCode)) {
    // Logging
    return nullptr;
  }

  return FileHandler::Ptr(new FileHandler(std::move(filepath), 0));
}

FileHandler::Ptr FileHandler::createFileFromBuffer(fs::path filepath, const std::vector<char>& buffer) {
  auto fileHandler = createEmptyFile(filepath);
  if(fileHandler && buffer.empty()) {
    return fileHandler;
  }

  std::ofstream oStream(filepath, std::ios::binary);
  oStream.write(buffer.data(), static_cast<int64_t>(buffer.size()));
  oStream.close();

  std::error_code errorCode;
  if(auto fileSize = fs::file_size(filepath, errorCode) != buffer.size()) {
    return nullptr;
  }

  fileHandler->mFileSize = buffer.size();
  return fileHandler;
}

FileHandler::Ptr FileHandler::createFileWithRandomData(fs::path filepath, size_t bufferSize) {
  return createFileFromBuffer(std::move(filepath), generateRandomBuffer(bufferSize));
}

std::vector<char> FileHandler::generateRandomBuffer(size_t bufferSize) {
  std::random_device random;
  std::mt19937 gen(random());
  std::uniform_int_distribution<short> distribution(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());

  std::vector<char> buffer(bufferSize);
  std::for_each(
      std::begin(buffer), std::end(buffer), [&distribution, &gen](auto& value) { value = static_cast<char>(distribution(gen)); });
  return buffer;
}

FileHandler::~FileHandler() {
  std::error_code errorCode;
  if(fs::exists(mFilePath, errorCode)) {
    fs::remove(mFilePath, errorCode);
  }
}

FileHandler::FileHandler(fs::path filePath, size_t bufferSize) : mFilePath{std::move(filePath)}, mFileSize{bufferSize} {}
