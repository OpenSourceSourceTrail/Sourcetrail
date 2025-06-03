#pragma once
#include <QWidget>

#include "FilePath.h"
#include "QtHelpButton.h"
#include "QtProjectWizardWindow.h"
#include "QtThreadedFunctor.h"

QT_FORWARD_DECLARE_CLASS(QFrame);
QT_FORWARD_DECLARE_CLASS(QGridLayout);
QT_FORWARD_DECLARE_CLASS(QLabel);
QT_FORWARD_DECLARE_CLASS(QToolButton);

class QtTextEditDialog;

class QtProjectWizardContent : public QWidget {
  Q_OBJECT

public:
  explicit QtProjectWizardContent(QtProjectWizardWindow* window);

  virtual void populate(QGridLayout* layout, int& row);
  virtual void windowReady();

  virtual void load();
  virtual void save();
  virtual void refresh();
  virtual bool check();

  [[nodiscard]] virtual std::vector<FilePath> getFilePaths() const;
  [[nodiscard]] virtual QString getFileNamesTitle() const;
  [[nodiscard]] virtual QString getFileNamesDescription() const;

  [[nodiscard]] bool isRequired() const;
  void setIsRequired(bool isRequired);

protected:
  [[nodiscard]] QLabel* createFormTitle(const QString& name) const;
  [[nodiscard]] QLabel* createFormLabel(QString name) const;
  [[nodiscard]] QLabel* createFormSubLabel(const QString& name) const;
  [[nodiscard]] QToolButton* createSourceGroupButton(const QString& name, const QString& iconPath) const;

  [[nodiscard]] QtHelpButton* addHelpButton(const QString& helpTitle, const QString& helpText, QGridLayout* layout, int row) const;
  [[nodiscard]] QPushButton* addFilesButton(const QString& name, QGridLayout* layout, int row) const;
  [[nodiscard]] QFrame* addSeparator(QGridLayout* layout, int row) const;

  QtProjectWizardWindow* mWindow;

  QtTextEditDialog* mFilesDialog = nullptr;

protected slots:
  void filesButtonClicked();
  void closedFilesDialog();

private:
  void showFilesDialog(const std::vector<FilePath>& filePaths);

  QtThreadedFunctor<const std::vector<FilePath>&> mShowFilesFunctor;

  bool mIsRequired = false;
};
