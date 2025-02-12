#include "FileSystem.h"

#include <algorithm>
#include <tuple>

#include <boost/date_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/filesystem.hpp>
// internal
#include "utilityString.h"

namespace {

bool isSelfReferencingSymlink(const boost::filesystem::path& symlink,
                              const boost::filesystem::recursive_directory_iterator& iterator) {
  return symlink.filename() == symlink.string() && symlink.filename() == iterator->path().filename();
}

bool isValidExtension(const std::wstring& extension, const std::set<std::wstring>& extensions) {
  return extensions.empty() || extensions.find(utility::toLowerCase(extension)) != extensions.end();
}

void processRegularFile(const FilePath& path,
                        const std::set<std::wstring>& extensions,
                        std::set<FilePath>& filePaths,
                        std::vector<FileInfo>& files) {
  if(!isValidExtension(path.extension(), extensions)) {
    return;
  }

  const FilePath canonicalPath = path.getCanonical();
  if(filePaths.find(canonicalPath) != filePaths.end()) {
    return;
  }

  filePaths.insert(canonicalPath);
  files.push_back(FileSystem::getFileInfoForPath(canonicalPath));
}

bool processSymlinkDirectory(boost::filesystem::recursive_directory_iterator& iterator,
                             const boost::filesystem::path& symlink,
                             std::set<boost::filesystem::path>& symlinkDirs) {
  const boost::filesystem::path absDir = boost::filesystem::canonical(symlink, iterator->path().parent_path());

  if(symlinkDirs.find(absDir) != symlinkDirs.end()) {
    iterator.disable_recursion_pending();
    return false;
  }

  symlinkDirs.insert(absDir);
  return true;
}

bool handleSymlink(boost::filesystem::recursive_directory_iterator& iterator,
                   std::set<boost::filesystem::path>& symlinkDirs,
                   bool followSymLinks) {
  if(!followSymLinks) {
    iterator.disable_recursion_pending();
    return false;
  }

  const boost::filesystem::path symlink = boost::filesystem::read_symlink(*iterator);
  if(isSelfReferencingSymlink(symlink, iterator)) {
    return false;
  }

  if(!boost::filesystem::is_directory(*iterator)) {
    return true;
  }

  return processSymlinkDirectory(iterator, symlink, symlinkDirs);
}

void processDirectory(const FilePath& path,
                      const std::set<std::wstring>& extensions,
                      std::set<boost::filesystem::path>& symlinkDirs,
                      std::set<FilePath>& filePaths,
                      std::vector<FileInfo>& files,
                      bool followSymLinks) {
  boost::filesystem::recursive_directory_iterator iterator{path.getPath(), boost::filesystem::directory_options::none};
  const boost::filesystem::recursive_directory_iterator endIterator;
  boost::system::error_code errorCode;

  for(; iterator != endIterator; iterator.increment(errorCode)) {
    if(boost::filesystem::is_symlink(*iterator)) {
      if(!handleSymlink(iterator, symlinkDirs, followSymLinks)) {
        continue;
      }
    }

    if(!boost::filesystem::is_regular_file(*iterator)) {
      continue;
    }

    processRegularFile(FilePath(iterator->path().wstring()), extensions, filePaths, files);
  }
}

}    // namespace

std::vector<FilePath> FileSystem::getFilePathsFromDirectory(const FilePath& path, const std::vector<std::wstring>& extensions) {
  std::set<std::wstring> ext(extensions.begin(), extensions.end());
  std::vector<FilePath> files;

  if(path.isDirectory()) {
    boost::filesystem::recursive_directory_iterator iterator(path.getPath());
    const boost::filesystem::recursive_directory_iterator endit;
    while(iterator != endit) {
      if(boost::filesystem::is_symlink(*iterator)) {
        // check for self-referencing symlinks
        const boost::filesystem::path symlink = boost::filesystem::read_symlink(*iterator);
        if(symlink.filename() == symlink.string() && symlink.filename() == iterator->path().filename()) {
          ++iterator;
          continue;
        }
      }

      if(boost::filesystem::is_regular_file(*iterator) &&
         (ext.empty() || ext.find(iterator->path().extension().wstring()) != ext.end())) {
        files.emplace_back(iterator->path().generic_wstring());
      }
      ++iterator;
    }
  }
  return files;
}

FileInfo FileSystem::getFileInfoForPath(const FilePath& filePath) {
  if(filePath.exists()) {
    return {filePath, getLastWriteTime(filePath)};
  }
  return {};
}

std::vector<FileInfo> FileSystem::getFileInfosFromPaths(const std::vector<FilePath>& paths,
                                                        const std::vector<std::wstring>& fileExtensions,
                                                        bool followSymLinks) {
  // Convert extensions to lowercase for case-insensitive comparison
  std::set<std::wstring> extensions;
  std::transform(fileExtensions.cbegin(),
                 fileExtensions.end(),
                 std::inserter(extensions, extensions.begin()),
                 static_cast<std::wstring (*)(const std::wstring&)>(utility::toLowerCase));

  // First iterator will fill symlinkDirs
  std::set<boost::filesystem::path> symlinkDirs;
  std::set<FilePath> filePaths;
  std::vector<FileInfo> files;

  for(const FilePath& path : paths) {
    if(!path.exists()) {
      continue;
    }

    if(!path.isDirectory()) {
      processRegularFile(path, extensions, filePaths, files);
      continue;
    }

    processDirectory(path, extensions, symlinkDirs, filePaths, files, followSymLinks);
  }

  for(const auto& dir : symlinkDirs) {
    processDirectory(FilePath{dir.wstring()}, extensions, symlinkDirs, filePaths, files, false);
  }

  return files;
}

std::set<FilePath> FileSystem::getSymLinkedDirectories(const FilePath& path) {
  return getSymLinkedDirectories(std::vector<FilePath>{path});
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): It will be fixed
std::set<FilePath> FileSystem::getSymLinkedDirectories(const std::vector<FilePath>& paths) {
  std::set<boost::filesystem::path> symlinkDirs;

  for(const FilePath& path : paths) {
    if(path.isDirectory()) {
      boost::filesystem::recursive_directory_iterator iterator(path.getPath(), boost::filesystem::directory_options::none);
      const boost::filesystem::recursive_directory_iterator endit;
      boost::system::error_code errorCode;
      for(; iterator != endit; iterator.increment(errorCode)) {
        if(boost::filesystem::is_symlink(*iterator)) {
          // check for self-referencing symlinks
          const boost::filesystem::path symlink = boost::filesystem::read_symlink(*iterator);
          if(symlink.filename() == symlink.string() && symlink.filename() == iterator->path().filename()) {
            continue;
          }

          // check for duplicates when following directory symlinks
          if(boost::filesystem::is_directory(*iterator)) {
            const boost::filesystem::path absDir = boost::filesystem::canonical(symlink, iterator->path().parent_path());

            if(symlinkDirs.find(absDir) != symlinkDirs.end()) {
              iterator.disable_recursion_pending();
              continue;
            }

            symlinkDirs.insert(absDir);
          }
        }
      }
    }
  }

  std::set<FilePath> files;
  for(const auto& symlinkDir : symlinkDirs) {
    files.insert(FilePath(symlinkDir.wstring()));
  }
  return files;
}

unsigned long long FileSystem::getFileByteSize(const FilePath& filePath) {
  return boost::filesystem::file_size(filePath.getPath());
}

TimeStamp FileSystem::getLastWriteTime(const FilePath& filePath) {
  boost::posix_time::ptime lastWriteTime;
  if(filePath.exists()) {
    const std::time_t temp = boost::filesystem::last_write_time(filePath.getPath());
    lastWriteTime = boost::posix_time::from_time_t(temp);
    lastWriteTime = boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local(lastWriteTime);
  }
  return {lastWriteTime};
}

bool FileSystem::remove(const FilePath& path) {
  boost::system::error_code errorCode;
  const bool ret = boost::filesystem::remove(path.getPath(), errorCode);
  std::ignore = path.recheckExists();
  return ret;
}

bool FileSystem::rename(const FilePath& fromPath, const FilePath& toPath) {
  if(!fromPath.recheckExists() || toPath.recheckExists()) {
    return false;
  }

  boost::filesystem::rename(fromPath.getPath(), toPath.getPath());
  std::ignore = toPath.recheckExists();
  return true;
}

bool FileSystem::copyFile(const FilePath& fromFile, const FilePath& toFile) {
  if(!fromFile.recheckExists() || toFile.recheckExists()) {
    return false;
  }

  boost::filesystem::copy_file(fromFile.getPath(), toFile.getPath());
  std::ignore = toFile.recheckExists();
  return true;
}

bool FileSystem::copy_directory(const FilePath& fromDir, const FilePath& toDir) {
  if(!fromDir.recheckExists() || toDir.recheckExists()) {
    return false;
  }

  boost::filesystem::create_directory(fromDir.getPath(), toDir.getPath());
  std::ignore = toDir.recheckExists();
  return true;
}

void FileSystem::createDirectory(const FilePath& path) {
  boost::filesystem::create_directories(path.str());
  std::ignore = path.recheckExists();
}

std::vector<FilePath> FileSystem::getDirectSubDirectories(const FilePath& path) {
  std::vector<FilePath> output;

  if(path.exists() && path.isDirectory()) {
    const boost::filesystem::directory_iterator end;
    for(boost::filesystem::directory_iterator dir{path.str()}; dir != end; dir++) {
      if(boost::filesystem::is_directory(dir->path())) {
        output.emplace_back(dir->path().wstring());
      }
    }
  }

  return output;
}

std::vector<FilePath> FileSystem::getRecursiveSubDirectories(const FilePath& path) {
  std::vector<FilePath> output;

  if(path.exists() && path.isDirectory()) {
    const boost::filesystem::recursive_directory_iterator end;
    for(boost::filesystem::recursive_directory_iterator dir{path.str()}; dir != end; dir++) {
      if(boost::filesystem::is_directory(dir->path())) {
        output.emplace_back(dir->path().wstring());
      }
    }
  }

  return output;
}