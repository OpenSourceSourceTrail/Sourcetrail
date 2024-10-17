#ifndef QT_PREFERENCES_WINDOW_H
#define QT_PREFERENCES_WINDOW_H

#include "details/ApplicationSettings.h"
#include "QtProjectWizardWindow.h"

class QtPreferencesWindow : public QtProjectWizardWindow {
  Q_OBJECT

public:
  QtPreferencesWindow(QWidget* parent = 0);
  virtual ~QtPreferencesWindow();

protected:
  virtual void windowReady() override;
  virtual void handleNext() override;

private:
  ApplicationSettings m_appSettings;    // NOTE(Hussein): Why we need instance not use the singleton
};

#endif    // QT_PREFERENCES_WINDOW_H
