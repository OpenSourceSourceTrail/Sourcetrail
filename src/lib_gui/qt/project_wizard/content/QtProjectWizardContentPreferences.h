#pragma once

#include <QComboBox>

#include "CombinedPathDetector.h"
#include "QtLocationPicker.h"
#include "QtPathListBox.h"
#include "QtProjectWizardContent.h"

class QCheckBox;
class QFontComboBox;
class QLabel;
class QLineEdit;


class QtComboBoxPlaceHolder : public QComboBox {
  Q_OBJECT

signals:
  void opened();

public:
  void showPopup() override {
    emit opened();
  }
};


class QtProjectWizardContentPreferences : public QtProjectWizardContent {
  Q_OBJECT

public:
  explicit QtProjectWizardContentPreferences(QtProjectWizardWindow* window);
  ~QtProjectWizardContentPreferences() override;

  // QtProjectWizardContent implementation
  void populate(QGridLayout* layout, int& row) override;

  void load() override;
  void save() override;
  bool check() override;

private slots:
  void colorSchemeChanged(int index);
  void loggingEnabledChanged();
  void indexerThreadsChanges(int index);
  void uiAutoScalingChanges(int index);
  void uiScaleFactorChanges(int index);

private:
  void addTitle(const QString& title, QGridLayout* layout, int& row);
  void addLabel(const QString& label, QGridLayout* layout, int row);
  void addWidget(QWidget* widget, QGridLayout* layout, int row, Qt::Alignment widgetAlignment = Qt::Alignment());
  void addLabelAndWidget(
      const QString& label, QWidget* widget, QGridLayout* layout, int row, Qt::Alignment widgetAlignment = Qt::Alignment());
  void addGap(QGridLayout* layout, int& row);

  QCheckBox* addCheckBox(const QString& label, const QString& text, const QString& helpText, QGridLayout* layout, int& row);
  QComboBox* addComboBox(const QString& label, const QString& helpText, QGridLayout* layout, int& row);
  QComboBox* addComboBoxWithWidgets(
      const QString& label, const QString& helpText, const std::vector<QWidget*>& widgets, QGridLayout* layout, int& row);
  QComboBox* addComboBox(const QString& label, int min, int max, const QString& helpText, QGridLayout* layout, int& row);
  QComboBox* addComboBoxWithWidgets(
      const QString& label, int min, int max, const QString& helpText, std::vector<QWidget*> widgets, QGridLayout* layout, int& row);
  QLineEdit* addLineEdit(const QString& label, const QString& helpText, QGridLayout* layout, int& row);

  QFontComboBox* m_fontFace;
  QtComboBoxPlaceHolder* m_fontFacePlaceHolder;

  QComboBox* m_fontSize;
  QComboBox* m_tabWidth;
  QComboBox* m_textEncoding;

  QComboBox* m_colorSchemes;
  std::vector<FilePath> m_colorSchemePaths;
  int m_oldColorSchemeIndex;
  int m_newColorSchemeIndex;

  QCheckBox* m_useAnimations;
  QCheckBox* m_showBuiltinTypes;
  QCheckBox* m_showDirectoryInCode;

  QComboBox* m_screenAutoScaling;
  QLabel* m_screenAutoScalingInfoLabel;

  QComboBox* m_screenScaleFactor;
  QLabel* m_screenScaleFactorInfoLabel;

  QLineEdit* m_scrollSpeed;
  QCheckBox* m_graphZooming;

  QCheckBox* m_loggingEnabled;
  QCheckBox* m_verboseIndexerLoggingEnabled;
  QtLocationPicker* m_logPath;

  QLineEdit* m_sourcetrailPort;
  QLineEdit* m_pluginPort;

  QComboBox* m_threads;
  QLabel* m_threadsInfoLabel;

  QCheckBox* m_multiProcessIndexing;
  QComboBox* mLoggingLevelComboBox;
};