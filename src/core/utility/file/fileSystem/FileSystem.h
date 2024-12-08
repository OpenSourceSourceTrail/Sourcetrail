#pragma once
#include <set>
#include <string>
#include <vector>

#include "FileInfo.h"
#include "TimeStamp.h"

namespace filesystem {
std::vector<FilePath> getFilePathsFromDirectory(const FilePath& path, const std::vector<std::wstring>& extensions = {});

FileInfo getFileInfoForPath(const FilePath& filePath);

std::vector<FileInfo> getFileInfosFromPaths(const std::vector<FilePath>& paths,
                                            const std::vector<std::wstring>& fileExtensions,
                                            bool followSymLinks = true);

std::set<FilePath> getSymLinkedDirectories(const FilePath& path);
std::set<FilePath> getSymLinkedDirectories(const std::vector<FilePath>& paths);
unsigned long long getFileByteSize(const FilePath& filePath);
TimeStamp getLastWriteTime(const FilePath& filePath);
bool remove(const FilePath& path);
bool rename(const FilePath& from, const FilePath& to);
bool copyFile(const FilePath& from, const FilePath& to);
void createDirectory(const FilePath& path);
std::vector<FilePath> getDirectSubDirectories(const FilePath& path);
bool isPortableName(const std::string& name);
bool isPortableFileName(const std::string& fileName);
}    // namespace filesystem
