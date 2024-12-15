#include "FileSystem.h"
// boost
#include <boost/date_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/filesystem.hpp>
// internal
#include "utilityString.h"

namespace file {
static bool doesSelfReferencingSymlinksExist(const boost::filesystem::recursive_directory_iterator& it,
                                             const boost::filesystem::path& p) {
  return (p.filename() == p.string()) && (p.filename() == it->path().filename());
}

std::vector<FilePath> getFilePathsFromDirectory(const FilePath& path, const std::vector<std::wstring>& extensions) {
  std::set<std::wstring> ext(extensions.begin(), extensions.end());
  std::vector<FilePath> files;

  if(path.isDirectory()) {
    boost::filesystem::recursive_directory_iterator it(path.getPath());
    boost::filesystem::recursive_directory_iterator endit;
    while(it != endit) {
      if(boost::filesystem::is_symlink(*it)) {
        // check for self-referencing symlinks
        boost::filesystem::path p = boost::filesystem::read_symlink(*it);
        if(doesSelfReferencingSymlinksExist(it, p)) {
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

FileInfo getFileInfoForPath(const FilePath& filePath) {
  if(filePath.exists()) {
    return FileInfo(filePath, getLastWriteTime(filePath));
  }
  return FileInfo();
}

std::vector<FileInfo> getFileInfosFromPaths(const std::vector<FilePath>& paths,
                                            const std::vector<std::wstring>& fileExtensions,
                                            bool followSymLinks) {
  std::set<std::wstring> ext(fileExtensions.begin(), fileExtensions.end());

  std::set<boost::filesystem::path> symlinkDirs;
  std::set<FilePath> filePaths;

  std::vector<FileInfo> files;

  for(const FilePath& path : paths) {
    if(path.isDirectory()) {
      boost::filesystem::recursive_directory_iterator it(
          path.getPath(), boost::filesystem::directory_options::follow_directory_symlink);
      boost::filesystem::recursive_directory_iterator endit;
      boost::system::error_code ec;
      for(; it != endit; it.increment(ec)) {
        if(boost::filesystem::is_symlink(*it)) {
          if(!followSymLinks) {
            it.no_push();
            continue;
          }

          // check for self-referencing symlinks
          boost::filesystem::path p = boost::filesystem::read_symlink(*it);
          if(doesSelfReferencingSymlinksExist(it, p)) {
            it.no_push();
            continue;
          }

          // check for duplicates when following directory symlinks
          if(boost::filesystem::is_directory(*it)) {
            boost::filesystem::path absDir = boost::filesystem::canonical(p, it->path().parent_path());

            if(symlinkDirs.find(absDir) != symlinkDirs.end()) {
              it.no_push();
              continue;
            }

            symlinkDirs.insert(absDir);
          }
        }

        if(boost::filesystem::is_regular_file(*it) && (ext.empty() || ext.find(it->path().extension().wstring()) != ext.end())) {
          const FilePath canonicalPath = FilePath(it->path().wstring()).getCanonical();
          if(filePaths.find(canonicalPath) != filePaths.end()) {
            continue;
          }
          filePaths.insert(canonicalPath);
          files.push_back(getFileInfoForPath(canonicalPath));
        }
      }
    } else if(path.exists() && (ext.empty() || ext.find(path.extension()) != ext.end())) {
      const FilePath canonicalPath = path.getCanonical();
      if(filePaths.find(canonicalPath) != filePaths.end()) {
        continue;
      }
      filePaths.insert(canonicalPath);
      files.push_back(getFileInfoForPath(canonicalPath));
    }
  }

  return files;
}

std::set<FilePath> getSymLinkedDirectories(const FilePath& path) {
  return getSymLinkedDirectories(std::vector<FilePath>{path});
}

std::set<FilePath> getSymLinkedDirectories(const std::vector<FilePath>& paths) {
  std::set<boost::filesystem::path> symlinkDirs;

  for(const FilePath& path : paths) {
    if(path.isDirectory()) {
      boost::filesystem::recursive_directory_iterator it(
          path.getPath(), boost::filesystem::directory_options::follow_directory_symlink);
      boost::filesystem::recursive_directory_iterator endit;
      boost::system::error_code ec;
      for(; it != endit; it.increment(ec)) {
        if(boost::filesystem::is_symlink(*it)) {
          // check for self-referencing symlinks
          boost::filesystem::path p = boost::filesystem::read_symlink(*it);
          if(doesSelfReferencingSymlinksExist(it, p)) {
            it.no_push();
            continue;
          }

          // check for duplicates when following directory symlinks
          if(boost::filesystem::is_directory(*it)) {
            boost::filesystem::path absDir = boost::filesystem::canonical(p, it->path().parent_path());

            if(symlinkDirs.find(absDir) != symlinkDirs.end()) {
              it.no_push();
              continue;
            }

            symlinkDirs.insert(absDir);
          }
        }
      }
    }
  }

  std::set<FilePath> files;
  std::transform(symlinkDirs.begin(), symlinkDirs.end(), std::inserter(files, files.end()), [](const auto& p) {
    return FilePath(p.wstring());
  });
  return files;
}

unsigned long long getFileByteSize(const FilePath& filePath) {
  return boost::filesystem::file_size(filePath.getPath());
}

TimeStamp getLastWriteTime(const FilePath& filePath) {
  boost::posix_time::ptime lastWriteTime;
  if(filePath.exists()) {
    std::time_t t = boost::filesystem::last_write_time(filePath.getPath());
    lastWriteTime = boost::posix_time::from_time_t(t);
    lastWriteTime = boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local(lastWriteTime);
  }
  return TimeStamp(lastWriteTime);
}

bool remove(const FilePath& path) {
  boost::system::error_code ec;
  const bool ret = boost::filesystem::remove(path.getPath(), ec);
  path.recheckExists();
  return ret;
}

bool rename(const FilePath& from, const FilePath& to) {
  if(!from.recheckExists() || to.recheckExists()) {
    return false;
  }

  boost::filesystem::rename(from.getPath(), to.getPath());

  return to.recheckExists();
}

bool copyFile(const FilePath& from, const FilePath& to) {
  if(!from.recheckExists() || to.recheckExists()) {
    return false;
  }

  boost::filesystem::copy_file(from.getPath(), to.getPath());

  return to.recheckExists();
}

void createDirectory(const FilePath& path) {
  boost::filesystem::create_directories(path.str());
  path.recheckExists();
}

std::vector<FilePath> getDirectSubDirectories(const FilePath& path) {
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

bool isPortableFileName(const std::string& fileName) {
  return boost::filesystem::portable_file_name(fileName);
}
}    // namespace file
