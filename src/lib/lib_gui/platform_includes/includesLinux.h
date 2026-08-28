#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>

#include "app/paths/AppPath.h"
#include "app/paths/ResourcePaths.h"
#include "app/paths/UserPaths.h"
#include "FilePath.h"
#include "qt/utility/utilityQt.h"
//
#include "settings/details/ApplicationSettings.h"

inline void setupPlatform(int /*argc*/, [[maybe_unused]] char* argv[]) {
  auto home = qEnvironmentVariable("HOME", "~");
  UserPaths::setUserDataDirectoryPath(FilePath{(home + "/.config/sourcetrail/").toStdString()});

  // Set QT screen scaling factor
  ApplicationSettings appSettings;
  appSettings.load(UserPaths::getAppSettingsFilePath(), true);

  qputenv("QT_AUTO_SCREEN_SCALE_FACTOR_SOURCETRAIL", qgetenv("QT_AUTO_SCREEN_SCALE_FACTOR"));
  qputenv("QT_SCALE_FACTOR_SOURCETRAIL", qgetenv("QT_SCALE_FACTOR"));

  if(const int autoScaling = appSettings.getScreenAutoScaling(); autoScaling != -1) {
    QByteArray bytes;
    bytes.setNum(autoScaling);
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", bytes);
  }

  if(const float scaleFactor = appSettings.getScreenScaleFactor(); scaleFactor > 0.0F) {
    QByteArray bytes;
    bytes.setNum(scaleFactor);
    qputenv("QT_SCALE_FACTOR", bytes);
  }
}

inline void setupApp([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  FilePath appPath = FilePath(QCoreApplication::applicationDirPath().toStdWString() + L"/").getAbsolute();
  AppPath::setSharedDataDirectoryPath(appPath);

  // Check if bundled as Linux AppImage
  if(appPath.getConcatenated(L"/../share/data").exists()) {
    AppPath::setSharedDataDirectoryPath(appPath.getConcatenated(L"/../share").getAbsolute());
  }


  QString userdir;
  if(auto value = qEnvironmentVariable("HOME"); !value.isEmpty()) {
    userdir = std::move(value);
  }
  userdir.append("/.config/sourcetrail/");

  QString userDataPath{userdir};
  QDir dataDir{userdir};
  if(!dataDir.exists()) {
    dataDir.mkpath(userDataPath);
  }

  utility::copyNewFilesFromDirectory(QString::fromStdWString(ResourcePaths::getFallbackDirectoryPath().wstr()), userDataPath);
  utility::copyNewFilesFromDirectory(
      QString::fromStdWString(AppPath::getSharedDataDirectoryPath().concatenate(L"user/").wstr()), userDataPath);

  // Add u+w permissions because the source files may be marked read-only in some distros
  QDirIterator dirIterator(userDataPath, QDir::Files, QDirIterator::Subdirectories);
  while(dirIterator.hasNext()) {
    QFile file(dirIterator.next());
    file.setPermissions(file.permissions() | QFile::WriteOwner);
  }
}