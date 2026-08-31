#pragma once
#include <QAbstractListModel>

#include "code/CodeSnapshot.h"

namespace code {

/**
 * The files currently shown -- the tab strip, and the header above each file's snippets.
 *
 * Not registered with QML; it reaches the scene as a QAbstractItemModel* on CodeViewModel. See the
 * note in graph/GraphModels.h for why registering it would not compile.
 */
class FileModel final : public QAbstractListModel {
  Q_OBJECT

public:
  enum Role : int {
    PathRole = Qt::UserRole + 1,
    NameRole,
    ReferenceCountRole,
    MinimizedRole,
    CompleteRole,
    SnippetCountRole,
  };

  explicit FileModel(QObject* parent = nullptr);
  ~FileModel() override;

  void setFiles(const QList<FileItem>& files);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
  QList<FileItem> mFiles;
};

/**
 * Every snippet on screen, flattened across files.
 *
 * Flat rather than nested because a ListView wants one row per delegate: a snippet carries the index
 * of the file it came from, and the panel draws a file header whenever that index changes. Source
 * locations are deliberately not roles here -- they are applied to the snippet's text document from
 * C++ (CodeViewModel::decorate), where the QTextCursor work belongs.
 */
class SnippetModel final : public QAbstractListModel {
  Q_OBJECT

public:
  enum Role : int {
    TitleRole = Qt::UserRole + 1,
    FooterRole,
    CodeRole,
    StartLineRole,
    EndLineRole,
    FileIndexRole,
    FilePathRole,
    FileNameRole,
    LanguageRole,
    FirstOfFileRole,
    IsOverviewRole,
  };

  explicit SnippetModel(QObject* parent = nullptr);
  ~SnippetModel() override;

  void setFiles(const QList<FileItem>& files);

  /** The spans for one row, for decoration and hit-testing. */
  [[nodiscard]] const QList<LocationSpan>& locationsAt(int row) const;
  [[nodiscard]] QString languageAt(int row) const;

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
  struct Row final {
    SnippetItem snippet;
    QString filePath;
    QString fileName;
    QString language;
    int fileIndex = 0;
    bool firstOfFile = false;
  };

  QList<Row> mRows;
};

}    // namespace code
