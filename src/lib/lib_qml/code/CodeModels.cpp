#include "code/CodeModels.h"

#include <QVariant>

namespace code {

namespace {

const QList<LocationSpan> kNoLocations;

}    // namespace

FileModel::FileModel(QObject* parent) : QAbstractListModel(parent) {}

FileModel::~FileModel() = default;

void FileModel::setFiles(const QList<FileItem>& files) {
  beginResetModel();
  mFiles = files;
  endResetModel();
}

int FileModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(mFiles.size());
}

QVariant FileModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() < 0 || index.row() >= mFiles.size()) {
    return {};
  }
  const FileItem& file = mFiles.at(index.row());
  switch(role) {
  case PathRole:
    return file.filePath;
  case NameRole:
    return file.fileName;
  case ReferenceCountRole:
    return file.referenceCount;
  case MinimizedRole:
    return file.isMinimized;
  case CompleteRole:
    return file.isComplete;
  case SnippetCountRole:
    return static_cast<int>(file.snippets.size());
  default:
    return {};
  }
}

QHash<int, QByteArray> FileModel::roleNames() const {
  return {
      {PathRole, "filePath"},
      {NameRole, "fileName"},
      {ReferenceCountRole, "fileReferenceCount"},
      {MinimizedRole, "fileMinimized"},
      {CompleteRole, "fileComplete"},
      {SnippetCountRole, "fileSnippetCount"},
  };
}

SnippetModel::SnippetModel(QObject* parent) : QAbstractListModel(parent) {}

SnippetModel::~SnippetModel() = default;

void SnippetModel::setFiles(const QList<FileItem>& files) {
  beginResetModel();
  mRows.clear();
  for(int fileIndex = 0; fileIndex < files.size(); ++fileIndex) {
    const FileItem& file = files.at(fileIndex);
    bool first = true;
    for(const SnippetItem& snippet : file.snippets) {
      mRows.append(Row{snippet, file.filePath, file.fileName, file.language, fileIndex, first});
      first = false;
    }
  }
  endResetModel();
}

const QList<LocationSpan>& SnippetModel::locationsAt(int row) const {
  if(row < 0 || row >= mRows.size()) {
    return kNoLocations;
  }
  return mRows.at(row).snippet.locations;
}

QString SnippetModel::languageAt(int row) const {
  return (row < 0 || row >= mRows.size()) ? QString{} : mRows.at(row).language;
}

int SnippetModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(mRows.size());
}

QVariant SnippetModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() < 0 || index.row() >= mRows.size()) {
    return {};
  }
  const Row& row = mRows.at(index.row());
  switch(role) {
  case TitleRole:
    return row.snippet.title;
  case FooterRole:
    return row.snippet.footer;
  case CodeRole:
    return row.snippet.code;
  case StartLineRole:
    return row.snippet.startLine;
  case EndLineRole:
    return row.snippet.endLine;
  case FileIndexRole:
    return row.fileIndex;
  case FilePathRole:
    return row.filePath;
  case FileNameRole:
    return row.fileName;
  case LanguageRole:
    return row.language;
  case FirstOfFileRole:
    return row.firstOfFile;
  case IsOverviewRole:
    return row.snippet.isOverview;
  default:
    return {};
  }
}

QHash<int, QByteArray> SnippetModel::roleNames() const {
  return {
      {TitleRole, "snippetTitle"},
      {FooterRole, "snippetFooter"},
      {CodeRole, "snippetCode"},
      {StartLineRole, "snippetStartLine"},
      {EndLineRole, "snippetEndLine"},
      {FileIndexRole, "snippetFileIndex"},
      {FilePathRole, "snippetFilePath"},
      {FileNameRole, "snippetFileName"},
      {LanguageRole, "snippetLanguage"},
      {FirstOfFileRole, "snippetFirstOfFile"},
      {IsOverviewRole, "snippetIsOverview"},
  };
}

}    // namespace code
