#pragma once
// Qt5
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
// internal
#include "AppPath.h"
#include "FilePath.h"
#include "ResourcePaths.h"
#include "UserPaths.h"
#include "utilityQt.h"
//
#include "details/ApplicationSettings.h"

inline void setupPlatform(int /*argc*/, [[maybe_unused]] char* argv[]) {
  std::string home = std::getenv("HOME");
  UserPaths::setUserDataDirectoryPath(FilePath(home + "/.config/sourcetrail/"));

  // Set QT screen scaling factor
  ApplicationSettings appSettings;
  appSettings.load(UserPaths::getAppSettingsFilePath(), true);

  qputenv("QT_AUTO_SCREEN_SCALE_FACTOR_SOURCETRAIL", qgetenv("QT_AUTO_SCREEN_SCALE_FACTOR"));
  qputenv("QT_SCALE_FACTOR_SOURCETRAIL", qgetenv("QT_SCALE_FACTOR"));

  const int autoScaling = appSettings.getScreenAutoScaling();
  if(autoScaling != -1) {
    QByteArray bytes;
    bytes.setNum(autoScaling);
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", bytes);
  }

  const float scaleFactor = appSettings.getScreenScaleFactor();
  if(scaleFactor > 0.0F) {
    QByteArray bytes;
    bytes.setNum(scaleFactor);
    qputenv("QT_SCALE_FACTOR", bytes);
  }
}

inline void setupApp([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  FilePath appPath = FilePath(QCoreApplication::applicationDirPath().toStdWString() + L"/").getAbsolute();
  AppPath::setSharedDataDirectoryPath(appPath);
  AppPath::setCxxIndexerDirectoryPath(appPath);

  // Check if bundled as Linux AppImage
  if(appPath.getConcatenated(L"/../share/data").exists()) {
    AppPath::setSharedDataDirectoryPath(appPath.getConcatenated(L"/../share").getAbsolute());
  }

  std::string userdir(std::getenv("HOME"));
  userdir.append("/.config/sourcetrail/");

  QString userDataPath(userdir.c_str());
  QDir dataDir(userdir.c_str());
  if(!dataDir.exists()) {
    dataDir.mkpath(userDataPath);
  }

  utility::copyNewFilesFromDirectory(QString::fromStdWString(ResourcePaths::getFallbackDirectoryPath().wstr()), userDataPath);
  utility::copyNewFilesFromDirectory(
      QString::fromStdWString(AppPath::getSharedDataDirectoryPath().concatenate(L"user/").wstr()), userDataPath);

  // Add u+w permissions because the source files may be marked read-only in some distros
  QDirIterator it(userDataPath, QDir::Files, QDirIterator::Subdirectories);
  while(it.hasNext()) {
    QFile f(it.next());
    f.setPermissions(f.permissions() | QFile::WriteOwner);
  }
}