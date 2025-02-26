#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>

#include "AppPath.h"
#include "FilePath.h"
#include "ResourcePaths.h"
#include "UserPaths.h"
#include "utilityQt.h"
//
#include "details/ApplicationSettings.h"

inline void setupPlatform(int /*argc*/, [[maybe_unused]] char* argv[]) {
  auto home = qEnvironmentVariable("HOME", "~");
  UserPaths::setUserDataDirectoryPath(FilePath{(home + "/.config/sourcetrail/").toStdString()});

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