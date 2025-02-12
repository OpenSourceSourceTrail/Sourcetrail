#include "FileSystem.h"

#include <algorithm>

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
  boost::filesystem::path absDir = boost::filesystem::canonical(symlink, iterator->path().parent_path());

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

  boost::filesystem::path symlink = boost::filesystem::read_symlink(*iterator);
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
  boost::filesystem::recursive_directory_iterator endIterator;
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
    boost::filesystem::recursive_directory_iterator it(path.getPath());
    boost::filesystem::recursive_directory_iterator endit;
    while(it != endit) {
      if(boost::filesystem::is_symlink(*it)) {
        // check for self-referencing symlinks
        boost::filesystem::path p = boost::filesystem::read_symlink(*it);
        if(p.filename() == p.string() && p.filename() == it->path().filename()) {
          ++it;
          continue;
        }
      }

      if(boost::filesystem::is_regular_file(*it) && (ext.empty() || ext.find(it->path().extension().wstring()) != ext.end())) {
        files.push_back(FilePath(it->path().generic_wstring()));
      }
      ++it;
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

std::set<FilePath> FileSystem::getSymLinkedDirectories(const std::vector<FilePath>& paths) {
  std::set<boost::filesystem::path> symlinkDirs;

  for(const FilePath& path : paths) {
    if(path.isDirectory()) {
      boost::filesystem::recursive_directory_iterator iterator(path.getPath(), boost::filesystem::directory_options::none);
      boost::filesystem::recursive_directory_iterator endit;
      boost::system::error_code errorCode;
      for(; iterator != endit; iterator.increment(errorCode)) {
        if(boost::filesystem::is_symlink(*iterator)) {
          // check for self-referencing symlinks
          boost::filesystem::path symlink = boost::filesystem::read_symlink(*iterator);
          if(symlink.filename() == symlink.string() && symlink.filename() == iterator->path().filename()) {
            continue;
          }

          // check for duplicates when following directory symlinks
          if(boost::filesystem::is_directory(*iterator)) {
            boost::filesystem::path absDir = boost::filesystem::canonical(symlink, iterator->path().parent_path());

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
    std::time_t t = boost::filesystem::last_write_time(filePath.getPath());
    lastWriteTime = boost::posix_time::from_time_t(t);
    lastWriteTime = boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local(lastWriteTime);
  }
  return TimeStamp(lastWriteTime);
}

bool FileSystem::remove(const FilePath& path) {
  boost::system::error_code ec;
  const bool ret = boost::filesystem::remove(path.getPath(), ec);
  path.recheckExists();
  return ret;
}

bool FileSystem::rename(const FilePath& from, const FilePath& to) {
  if(!from.recheckExists() || to.recheckExists()) {
    return false;
  }

  boost::filesystem::rename(from.getPath(), to.getPath());
  to.recheckExists();
  return true;
}

bool FileSystem::copyFile(const FilePath& from, const FilePath& to) {
  if(!from.recheckExists() || to.recheckExists()) {
    return false;
  }

  boost::filesystem::copy_file(from.getPath(), to.getPath());
  to.recheckExists();
  return true;
}

bool FileSystem::copy_directory(const FilePath& from, const FilePath& to) {
  if(!from.recheckExists() || to.recheckExists()) {
    return false;
  }

  boost::filesystem::copy_directory(from.getPath(), to.getPath());
  to.recheckExists();
  return true;
}

void FileSystem::createDirectory(const FilePath& path) {
  boost::filesystem::create_directories(path.str());
  path.recheckExists();
}

std::vector<FilePath> FileSystem::getDirectSubDirectories(const FilePath& path) {
  std::vector<FilePath> v;

  if(path.exists() && path.isDirectory()) {
    for(boost::filesystem::directory_iterator end, dir(path.str()); dir != end; dir++) {
      if(boost::filesystem::is_directory(dir->path())) {
        v.push_back(FilePath(dir->path().wstring()));
      }
    }
  }

  return v;
}

std::vector<FilePath> FileSystem::getRecursiveSubDirectories(const FilePath& path) {
  std::vector<FilePath> v;

  if(path.exists() && path.isDirectory()) {
    for(boost::filesystem::recursive_directory_iterator end, dir(path.str()); dir != end; dir++) {
      if(boost::filesystem::is_directory(dir->path())) {
        v.push_back(FilePath(dir->path().wstring()));
      }
    }
  }

  return v;
}