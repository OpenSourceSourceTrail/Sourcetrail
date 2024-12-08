#include "FileSystem.h"
// boost
#include <boost/date_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>

// internal
#include <regex>

#include "logging.h"
#include "utilityString.h"

namespace fs = std::filesystem;

namespace filesystem {
static bool doesSelfReferencingSymlinksExist(const fs::path& symlinkPath, const fs::path& resolvedSymlinkPath) {
  return resolvedSymlinkPath.filename() == resolvedSymlinkPath.string() && resolvedSymlinkPath.filename() == symlinkPath.filename();
}

std::vector<FilePath> getFilePathsFromDirectory(const FilePath& path, const std::vector<std::wstring>& extensions) {
  std::set<std::wstring> ext(extensions.begin(), extensions.end());
  std::vector<FilePath> files;

  if(path.isDirectory()) {
    fs::recursive_directory_iterator it(path.getPath());
    fs::recursive_directory_iterator endit;
    while(it != endit) {
      if(fs::is_symlink(*it)) {
        // check for self-referencing symlinks
        fs::path p = fs::read_symlink(*it);
        if(doesSelfReferencingSymlinksExist(*it, p)) {
          ++it;
          continue;
        }
      }

      if(fs::is_regular_file(*it) && (ext.empty() || ext.find(it->path().extension().wstring()) != ext.end())) {
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
  std::set<std::wstring> ext;
  for(const std::wstring& e : fileExtensions) {
    ext.insert(utility::toLowerCase(e));
  }

  std::set<fs::path> symlinkDirs;
  std::set<FilePath> filePaths;

  std::vector<FileInfo> files;

  for(const FilePath& path : paths) {
    if(path.isDirectory()) {
      fs::recursive_directory_iterator it(path.getPath(), fs::directory_options::follow_directory_symlink);
      fs::recursive_directory_iterator endit;

      for(; it != endit; ++it) {
        if(fs::is_symlink(*it)) {
          if(!followSymLinks) {
            it.disable_recursion_pending();
            continue;
          }

          // check for self-referencing symlinks
          fs::path p = fs::read_symlink(*it);
          if(doesSelfReferencingSymlinksExist(*it, p)) {
            continue;
          }

          // check for duplicates when following directory symlinks
          if(fs::is_directory(*it)) {
            fs::path absDir = std::filesystem::canonical(it->path().parent_path() / p);

            if(symlinkDirs.find(absDir) != symlinkDirs.end()) {
              it.disable_recursion_pending();
              continue;
            }

            symlinkDirs.insert(absDir);
          }
        }

        if(fs::is_regular_file(*it) &&
           (ext.empty() || ext.find(utility::toLowerCase(it->path().extension().wstring())) != ext.end())) {
          const FilePath canonicalPath = FilePath(it->path().wstring()).getCanonical();
          if(filePaths.find(canonicalPath) != filePaths.end()) {
            continue;
          }
          filePaths.insert(canonicalPath);
          files.push_back(getFileInfoForPath(canonicalPath));
        }
      }
    } else if(path.exists() && (ext.empty() || ext.find(utility::toLowerCase(path.extension())) != ext.end())) {
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
  std::set<fs::path> symlinkDirs;

  for(const FilePath& path : paths) {
    if(path.isDirectory()) {
      fs::recursive_directory_iterator it(path.getPath(), fs::directory_options::follow_directory_symlink);
      fs::recursive_directory_iterator endit;
      for(; it != endit; ++it) {
        if(fs::is_symlink(*it)) {
          // check for self-referencing symlinks
          fs::path p = fs::read_symlink(*it);
          if(doesSelfReferencingSymlinksExist(*it, p)) {
            continue;
          }

          // check for duplicates when following directory symlinks
          if(fs::is_directory(*it)) {
            fs::path absDir = std::filesystem::canonical(it->path().parent_path() / p);

            if(symlinkDirs.find(absDir) != symlinkDirs.end()) {
              it.disable_recursion_pending();
              continue;
            }

            symlinkDirs.insert(absDir);
          }
        }
      }
    }
  }

  std::set<FilePath> files;
  for(auto& p : symlinkDirs) {
    files.insert(FilePath(p.wstring()));
  }
  return files;
}

unsigned long long getFileByteSize(const FilePath& filePath) {
  return fs::file_size(filePath.getPath());
}

TimeStamp getLastWriteTime(const FilePath& filePath) {
  if(filePath.exists()) {
    std::filesystem::file_time_type ft = std::filesystem::last_write_time(filePath.getPath());
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ft - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()));
    auto lastWriteTime = boost::posix_time::from_time_t(t);
    lastWriteTime = boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local(lastWriteTime);
    return TimeStamp(lastWriteTime);
  }
  return TimeStamp();
}

bool remove(const FilePath& path) {
  std::error_code ec;
  const bool ret = fs::remove(path.getPath(), ec);
  if(ec) {
    LOG_ERROR(ec.message());
  }
  path.recheckExists();
  return ret;
}

bool rename(const FilePath& from, const FilePath& to) {
  if(!from.recheckExists() || to.recheckExists()) {
    return false;
  }

  fs::rename(from.getPath(), to.getPath());

  return to.recheckExists();
}

bool copyFile(const FilePath& from, const FilePath& to) {
  if(!from.recheckExists() || to.recheckExists()) {
    return false;
  }

  fs::copy_file(from.getPath(), to.getPath());

  return to.recheckExists();
}

void createDirectory(const FilePath& path) {
  fs::create_directories(path.str());
  path.recheckExists();
}

std::vector<FilePath> getDirectSubDirectories(const FilePath& path) {
  std::vector<FilePath> v;

  if(path.exists() && path.isDirectory()) {
    for(fs::directory_iterator end, dir(path.str()); dir != end; dir++) {
      if(fs::is_directory(dir->path())) {
        v.push_back(FilePath(dir->path().wstring()));
      }
    }
  }

  return v;
}

inline bool isPortablePosixName(const std::string& name) {
  if(name.empty()) {
    return false;
  }

  const std::regex posixNameRegex("^[a-zA-Z0-9._-]+$");
  return std::regex_match(name, posixNameRegex);
}

inline bool isWindowsName(const std::string& name) {
  if(name.empty()) {
    return false;
  }
  const std::regex windowsNameRegex("^[\\x00-\\x1F<>:\"/\\\\|]+$");
  return !std::regex_match(name, windowsNameRegex) && (name == "." || name == ".." || (name.back() != ' ' && name.back() != '.'));
}

bool isPortableName(const std::string& name) {
  if(name.size() > 1 && (*name.begin() == '.' || *name.begin() == '-')) {
    return false;
  }
  return isPortablePosixName(name) && isWindowsName(name);
}

bool isPortableFileName(const std::string& fileName) {
  return isPortableName(fileName) && std::regex_match(fileName, std::regex(".*\\.[^\\.]{1,3}$"));
}
}    // namespace filesystem
