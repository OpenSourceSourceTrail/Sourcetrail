#include "Fonts.h"

#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QStringList>

#include "app/paths/ResourcePaths.h"
#include "logging.h"

namespace qml {

void loadApplicationFonts() {
  const QDir directory{QString::fromStdWString(ResourcePaths::getFontsDirectoryPath().wstr())};
  if(!directory.exists()) {
    LOG_WARNING(L"No fonts directory at " + directory.absolutePath().toStdWString());
    return;
  }

  // Read through QFile rather than handing QFontDatabase a path: the fonts directory is a symlink
  // into the resources submodule in a build tree, and addApplicationFont resolves paths itself.
  for(const QString& fileName : directory.entryList({QStringLiteral("*.ttf"), QStringLiteral("*.otf")}, QDir::Files)) {
    QFile file{directory.filePath(fileName)};
    if(!file.open(QIODevice::ReadOnly)) {
      LOG_WARNING(L"Failed to read font " + fileName.toStdWString());
      continue;
    }
    if(QFontDatabase::addApplicationFontFromData(file.readAll()) == -1) {
      LOG_WARNING(L"Failed to register font " + fileName.toStdWString());
    }
  }
}

}    // namespace qml
