#pragma once

#include "QtPathListBox.h"
#include "QtWindow.h"

class FilePath;

class QtPathListDialog : public QtWindow {
  Q_OBJECT

public:
  QtPathListDialog(QString title, QString description, QtPathListBox::SelectionPolicyType selectionPolicy, QWidget* parent = nullptr);

  [[nodiscard]] QSize sizeHint() const override;

  void setRelativeRootDirectory(const FilePath& dir);
  void setPaths(const std::vector<FilePath>& paths, bool readOnly = false);
  std::vector<FilePath> getPaths();

protected:
  void populateWindow(QWidget* widget) override;
  void windowReady() override;

  const QString m_title;
  const QString m_description;

private:
  const QtPathListBox::SelectionPolicyType m_selectionPolicy;
  QtPathListBox* m_pathList;
};