#pragma once
// Qt5
#include <QFutureWatcher>
#include <QWidget>
// internal
#include "FilePath.h"
#include "project/ui/QtProjectWizardWindow.h"
#include "qt/element/button/QtHelpButton.h"

QT_FORWARD_DECLARE_CLASS(QFrame);
QT_FORWARD_DECLARE_CLASS(QGridLayout);
QT_FORWARD_DECLARE_CLASS(QLabel);
QT_FORWARD_DECLARE_CLASS(QToolButton);

class QtTextEditDialog;

class QtProjectWizardContent : public QWidget {
  Q_OBJECT

public:
  QtProjectWizardContent(QtProjectWizardWindow* window);

  virtual void populate(QGridLayout* layout, int& row);
  virtual void windowReady();

  virtual void load();
  virtual void save();
  virtual void refresh();
  virtual bool check();

  virtual std::vector<FilePath> getFilePaths() const;
  virtual QString getFileNamesTitle() const;
  virtual QString getFileNamesDescription() const;

  bool isRequired() const;
  void setIsRequired(bool isRequired);

protected:
  QLabel* createFormTitle(const QString& name) const;
  QLabel* createFormLabel(QString name) const;
  QLabel* createFormSubLabel(const QString& name) const;
  QToolButton* createSourceGroupButton(const QString& name, const QString& iconPath) const;

  QtHelpButton* addHelpButton(const QString& helpTitle, const QString& helpText, QGridLayout* layout, int row) const;
  QPushButton* addFilesButton(const QString& name, QGridLayout* layout, int row) const;
  QFrame* addSeparator(QGridLayout* layout, int row) const;

  QtProjectWizardWindow* m_window;

  QtTextEditDialog* m_filesDialog = nullptr;

protected slots:
  void filesButtonClicked();
  void closedFilesDialog();

private:
  void showFilesDialog(const std::vector<FilePath>& filePaths);

  QFutureWatcher<std::vector<FilePath>> m_filePathsWatcher;

  bool m_isRequired = false;
};
