#include "utilityQt.h"

#include <set>

#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QIcon>
#include <QPainter>
#include <QWidget>

#include "ColorScheme.h"
#include "FilePath.h"
#include "FileSystem.h"
#include "IApplicationSettings.hpp"
#include "logging.h"
#include "QtMainView.h"
#include "ResourcePaths.h"
#include "TextAccess.h"
#include "utilityApp.h"
#include "utilityString.h"

namespace utility {

QIcon toIcon(const std::wstring& path) {
  return QIcon(QString::fromStdWString(ResourcePaths::getGuiDirectoryPath().concatenate(path).wstr()));
}

void setWidgetBackgroundColor(QWidget* widget, const std::string& color) {
  QPalette palette = widget->palette();
  palette.setColor(widget->backgroundRole(), QColor(color.c_str()));
  widget->setPalette(palette);
  widget->setAutoFillBackground(true);
}

void setWidgetRetainsSpaceWhenHidden(QWidget* widget) {
  QSizePolicy pol = widget->sizePolicy();
  pol.setRetainSizeWhenHidden(true);
  widget->setSizePolicy(pol);
}

void loadFontsFromDirectory(const FilePath& path, const std::wstring& extension) {
  std::vector<std::wstring> extensions;
  extensions.push_back(extension);
  const std::vector<FilePath> fontFilePaths = FileSystem::getFilePathsFromDirectory(path, extensions);

  std::set<int> loadedFontIds;

  for(const FilePath& fontFilePath : fontFilePaths) {
    QFile file(QString::fromStdWString(fontFilePath.wstr()));
    if(file.open(QIODevice::ReadOnly)) {
      const int fontId = QFontDatabase::addApplicationFontFromData(file.readAll());
      if(fontId != -1) {
        loadedFontIds.insert(fontId);
      }
    }
  }

  for(const int loadedFontId : loadedFontIds) {
    for(const QString& family : QFontDatabase::applicationFontFamilies(loadedFontId)) {
      LOG_INFO(L"Loaded FontFamily: " + family.toStdWString());
    }
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::string getStyleSheet(const FilePath& path) {
  std::string css = TextAccess::createFromFile(path)->getText();

  size_t pos = 0;

  while(pos != std::string::npos) {
    const size_t posA = css.find('<', pos);
    const size_t posB = css.find('>', pos);

    if(posA == std::string::npos || posB == std::string::npos) {
      break;
    }

    std::deque<std::string> seq = utility::split(css.substr(posA + 1, posB - posA - 1), ':');
    if(seq.size() != 2) {
      LOG_ERROR(L"Syntax error in file: " + path.wstr());
      return "";
    }

    const std::string key = seq.front();
    std::string val = seq.back();

    if(key == "setting") {
      if(val.find("font_size") != std::string::npos) {
        // check for modifier
        if(val.find('+') != std::string::npos) {
          const size_t findPos = val.find('+');
          const std::string sub = val.substr(findPos + 1);

          const int mod = std::stoi(sub);

          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize() + mod);
        } else if(val.find('-') != std::string::npos) {
          const size_t findPos = val.find('-');
          const std::string sub = val.substr(findPos + 1);

          const int mod = std::stoi(sub);

          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize() - mod);
        } else if(val.find('*') != std::string::npos) {
          const size_t findPos = val.find('*');
          const std::string sub = val.substr(findPos + 1);

          const int mod = std::stoi(sub);

          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize() * mod);
        } else if(val.find('/') != std::string::npos) {
          const size_t findPos = val.find('/');
          const std::string sub = val.substr(findPos + 1);

          const int mod = std::stoi(sub);

          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize() / mod);
        } else {
          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize());
        }
      } else if(val == "font_name") {
        val = IApplicationSettings::getInstanceRaw()->getFontName();
      } else if(val == "gui_path") {
        val = ResourcePaths::getGuiDirectoryPath().str();

        size_t index = 0;
        while(true) {
          index = val.find('\\', index);
          if(index == std::string::npos) {
            break;
          }
          val.replace(index, 1, "/");
          index += 3;
        }
      } else {
        LOG_ERROR(L"Syntax error in file: " + path.wstr());
        return "";
      }
    } else if(key == "color") {
      if(!ColorScheme::getInstance()->hasColor(val)) {
        LOG_WARNING("Color scheme does not provide value for key \"" + val + "\" requested by style \"" + path.str() + "\".");
      }
      val = ColorScheme::getInstance()->getColor(val);
    } else if(key == "platform_wml") {
      std::vector<std::string> values = utility::splitToVector(val, '|');
      if(values.size() != 3) {
        LOG_ERROR(L"Syntax error in file: " + path.wstr());
        return "";
      }

      switch(utility::getOsType()) {
      case OsType::Windows:
        val = values[0];
        break;
      case OsType::Mac:
        val = values[1];
        break;
      case OsType::Linux:
        val = values[2];
        break;
      default:
        break;
      }
    } else {
      LOG_ERROR(L"Syntax error in file: " + path.wstr());
      return "";
    }

    css.replace(posA, posB - posA + 1, val);
    pos = posA + val.size();
  }

  return css;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
QString getStyleSheet(const QString& resource) {
  QFile file(resource);
  if(!file.open(QIODevice::ReadOnly)) {
    LOG_WARNING(fmt::format("Failed to read {}", resource.toStdString()));
    return {};
  }
  std::string css = file.readAll().toStdString();

  size_t pos = 0;

  while(pos != std::string::npos) {
    const size_t posA = css.find('<', pos);
    const size_t posB = css.find('>', pos);

    if(posA == std::string::npos || posB == std::string::npos) {
      break;
    }

    std::deque<std::string> seq = utility::split(css.substr(posA + 1, posB - posA - 1), ':');
    if(seq.size() != 2) {
      LOG_ERROR(L"Syntax error in resource: " + resource.toStdWString());
      return "";
    }

    const std::string key = seq.front();
    std::string val = seq.back();

    if(key == "setting") {
      if(val.find("font_size") != std::string::npos) {
        // check for modifier
        if(val.find('+') != std::string::npos) {
          const size_t findPos = val.find('+');
          const std::string sub = val.substr(findPos + 1);

          const int mod = std::stoi(sub);

          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize() + mod);
        } else if(val.find('-') != std::string::npos) {
          const size_t findPos = val.find('-');
          const std::string sub = val.substr(findPos + 1);

          const int mod = std::stoi(sub);

          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize() - mod);
        } else if(val.find('*') != std::string::npos) {
          const size_t findPos = val.find('*');
          const std::string sub = val.substr(findPos + 1);

          const int mod = std::stoi(sub);

          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize() * mod);
        } else if(val.find('/') != std::string::npos) {
          const size_t findPos = val.find('/');
          const std::string sub = val.substr(findPos + 1);

          const int mod = std::stoi(sub);

          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize() / mod);
        } else {
          val = std::to_string(IApplicationSettings::getInstanceRaw()->getFontSize());
        }
      } else if(val == "font_name") {
        val = IApplicationSettings::getInstanceRaw()->getFontName();
      } else if(val == "gui_path") {
        val = ResourcePaths::getGuiDirectoryPath().str();

        size_t index = 0;
        while(true) {
          index = val.find('\\', index);
          if(index == std::string::npos) {
            break;
          }
          val.replace(index, 1, "/");
          index += 3;
        }
      } else {
        LOG_ERROR(L"Syntax error in resource: " + resource.toStdWString());
        return "";
      }
    } else if(key == "color") {
      if(!ColorScheme::getInstance()->hasColor(val)) {
        LOG_WARNING("Color scheme does not provide value for key \"" + val + "\" requested by style \"" + resource.toStdString() +
                    "\".");
      }
      val = ColorScheme::getInstance()->getColor(val);
    } else if(key == "platform_wml") {
      std::vector<std::string> values = utility::splitToVector(val, '|');
      if(values.size() != 3) {
        LOG_ERROR(L"Syntax error in resource: " + resource.toStdWString());
        return "";
      }

      switch(utility::getOsType()) {
      case OsType::Windows:
        val = values[0];
        break;
      case OsType::Mac:
        val = values[1];
        break;
      case OsType::Linux:
        val = values[2];
        break;
      default:
        break;
      }
    } else {
      LOG_ERROR(L"Syntax error in resource: " + resource.toStdWString());
      return "";
    }

    css.replace(posA, posB - posA + 1, val);
    pos = posA + val.size();
  }

  return QString::fromStdString(css);
}

QPixmap colorizePixmap(const QPixmap& pixmap, const QColor& color) {
  QImage image = pixmap.toImage();
  QImage colorImage(image.size(), image.format());
  QPainter colorPainter(&colorImage);
  colorPainter.fillRect(image.rect(), color);

  QPainter painter(&image);
  painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
  painter.drawImage(0, 0, colorImage);
  return QPixmap::fromImage(image);
}

QIcon createButtonIcon(const FilePath& iconPath, const std::string& colorId) {
  const ColorScheme* scheme = ColorScheme::getInstance().get();

  const QPixmap pixmap(QString::fromStdWString(iconPath.wstr()));
  QIcon icon(colorizePixmap(pixmap, scheme->getColor(colorId + "/icon").c_str()));

  if(scheme->hasColor(colorId + "/icon_disabled")) {
    icon.addPixmap(colorizePixmap(pixmap, scheme->getColor(colorId + "/icon_disabled").c_str()), QIcon::Disabled);
  }

  return icon;
}

QtMainWindow* getMainWindowforMainView(ViewLayout* viewLayout) {
  if(const auto* mainView = dynamic_cast<QtMainView*>(viewLayout); nullptr != mainView) {
    return mainView->getMainWindow();
  }
  return nullptr;
}

// NOLINTNEXTLINE(misc-no-recursion)
void copyNewFilesFromDirectory(const QString& src, const QString& dst) {
  const QDir dir(src);
  if(!dir.exists()) {
    return;
  }

  foreach(const QString currentDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    const QString dst_path = dst + QDir::separator() + currentDir;

    std::ignore = dir.mkpath(dst_path);
    copyNewFilesFromDirectory(src + QDir::separator() + currentDir, dst_path);
  }

  foreach(const QString currentFile, dir.entryList(QDir::Files)) {
    if(!QFile::exists(dst + QDir::separator() + currentFile)) {
      QFile::copy(src + QDir::separator() + currentFile, dst + QDir::separator() + currentFile);
    }
  }
}

}    // namespace utility