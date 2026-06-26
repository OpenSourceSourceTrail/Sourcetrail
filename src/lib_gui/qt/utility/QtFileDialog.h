#pragma once
#include <QStringList>

class FilePath;
class QString;
class QWidget;

class QtFileDialog {
public:
  static QStringList getFileNamesAndDirectories(QWidget* parent, const FilePath& path);

  static QString getExistingDirectory(QWidget* parent, const QString& caption, const FilePath& dir);
  static QString getOpenFileName(QWidget* parent, const QString& caption, const FilePath& dir, const QString& filter);

  static QString showSaveFileDialog(QWidget* parent, const QString& title, const FilePath& directory, const QString& filter);

private:
  static QString getDir(QString dir);
  static void saveFilePickerLocation(const FilePath& path);
};
