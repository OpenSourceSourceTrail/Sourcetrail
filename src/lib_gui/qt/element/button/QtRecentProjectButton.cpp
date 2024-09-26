#include "QtRecentProjectButton.h"
// STL
#include <algorithm>
#include <iterator>
// Boost
#include <boost/filesystem/path.hpp>
// Qt5
#include <QAction>
#include <QMessageBox>
#include <QString>
// internal
#include "MessageLoadProject.h"
#include "QtContextMenu.h"
#include "UserPaths.h"
#include "IApplicationSettings.hpp"

QtRecentProjectButton::QtRecentProjectButton(QWidget* pParent) : QPushButton(pParent) {}

bool QtRecentProjectButton::projectExists() const {
  return m_projectExists;
}

void QtRecentProjectButton::setProjectPath(const FilePath& projectFilePath) {
  m_projectFilePath = projectFilePath;
  m_projectExists = projectFilePath.exists();
  this->setText(QString::fromStdWString(m_projectFilePath.withoutExtension().fileName()));
  if(m_projectExists) {
    this->setToolTip(QString::fromStdWString(m_projectFilePath.wstr()));
  } else {
    const std::wstring missingFileText = L"Couldn't find " + m_projectFilePath.wstr() + L" in your filesystem";
    this->setToolTip(QString::fromStdWString(missingFileText));
  }
}

void QtRecentProjectButton::handleButtonClick() {
  if(m_projectExists) {
    MessageLoadProject(m_projectFilePath).dispatch();
  } else {
    QMessageBox msgBox;
    msgBox.setText(QStringLiteral("Missing Project File"));
    msgBox.setInformativeText(
        QString::fromStdWString(L"<p>Couldn't find \"" + m_projectFilePath.wstr() +
                                L"\" on your filesystem.</p><p>Do you want to remove it from recent project "
                                L"list?</p>"));
    msgBox.addButton(QStringLiteral("Remove"), QMessageBox::ButtonRole::YesRole);
    msgBox.addButton(QStringLiteral("Keep"), QMessageBox::ButtonRole::NoRole);
    msgBox.setIcon(QMessageBox::Icon::Question);
    const int ret = msgBox.exec();

    // QMessageBox::Yes
    if(ret == 0) {
      auto recentProjects = IApplicationSettings::getInstanceRaw()->getRecentProjects();
      for(size_t i = 0; i < recentProjects.size(); ++i) {
        if(recentProjects[i].wstr() == m_projectFilePath.wstr()) {
          recentProjects.erase(recentProjects.begin() + static_cast<long>(i));
          IApplicationSettings::getInstanceRaw()->setRecentProjects(recentProjects);
          IApplicationSettings::getInstanceRaw()->save();
          break;
        }
      }
      emit updateButtons();
    }
  }
}

void QtRecentProjectButton::contextMenuEvent(QContextMenuEvent* pEvent) {
  QtContextMenu menu(pEvent, this);

  QAction deleteAction("delete");
  connect(&deleteAction, &QAction::triggered, [this]() {
    auto recentProjects = IApplicationSettings::getInstanceRaw()->getRecentProjects();
    // Remove the current project from RecentProjects
    recentProjects.erase(std::remove_if(std::begin(recentProjects),
                                        std::end(recentProjects),
                                        [projectExists = m_projectFilePath](const auto& currentProject) {
                                          return projectExists.getPath() == currentProject.getPath();
                                        }),
                         std::end(recentProjects));
    recentProjects.erase(std::remove_if(std::begin(recentProjects),
                                        std::end(recentProjects),
                                        [projectExists = m_projectFilePath](const auto& currentProject) {
                                          return projectExists.getPath() == currentProject.getPath();
                                        }),
                         std::end(recentProjects));
    // Update RecentProjects
    auto pApplicationSettings = IApplicationSettings::getInstanceRaw();
    pApplicationSettings->setRecentProjects(recentProjects);
    pApplicationSettings->save(UserPaths::getAppSettingsFilePath());
    hide();

    emit updateButtons();
  });

  menu.addAction(&deleteAction);
  menu.show();
}
