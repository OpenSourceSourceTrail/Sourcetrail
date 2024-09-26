#include "QtStartScreen.hpp"

#include <utility>

#include <fmt/format.h>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include <QDebug>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QUrl>

#include "RecentItemModel.hpp"
#include "UserPaths.h"
#include "Version.h"
#include "IApplicationSettings.hpp"
#include "globalStrings.h"
#include "utilityQt.h"

namespace {

QPushButton* createButton(qt::window::QtStartScreen* that,
                          const QString& buttonName,
                          const QString& objectName,
                          std::function<void()> onClickEvent) {
  auto* newProjectButton = new QPushButton(buttonName, that);    // NOLINT(cppcoreguidelines-owning-memory)
#if Q_OS_MAC
  newProjectButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);    // fixes layouting on Mac
#endif
  newProjectButton->setObjectName(objectName);
  newProjectButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QObject::connect(newProjectButton, &QPushButton::clicked, that, std::move(onClickEvent));
  return newProjectButton;
}

}    // namespace

namespace qt::window {

QtStartScreen::QtStartScreen(QWidget* pParent) : QtWindow(true, pParent) {}

QSize QtStartScreen::sizeHint() const {
  constexpr auto Size = QSize(600, 650);
  return Size;
}

void QtStartScreen::setupStartScreen() {
  addLogo();

  // Create the main layout
  auto* layout = new QHBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  constexpr QMargins LayoutMargins {15, 170, 15, 0};
  layout->setContentsMargins(LayoutMargins);
  m_content->setLayout(layout);

  createVersionAndGithub(layout);

  constexpr auto LayoutSpacing = 50;
  layout->addSpacing(LayoutSpacing);

  createRecentProjects(layout);

  // Move the window to center of the parent window.
  const QSize size = sizeHint();
  move(parentWidget()->width() / 2 - size.width() / 2, parentWidget()->height() / 2 - size.height() / 2);

  setStyleSheet(utility::getStyleSheet("://startscreen/startscreen.css"));
}

void QtStartScreen::hideEvent(QHideEvent* hideEvent) {
  if(mRecentModel->isDirty()) {
    auto updatedRecentProjects = mRecentModel->getRecentProjects() | ranges::views::transform([](auto item)-> std::filesystem::path {
                                   return item.path.wstring();
                                 }) |
        ranges::to<std::vector>();

    IApplicationSettings::getInstanceRaw()->setRecentProjects(updatedRecentProjects);
    if(IApplicationSettings::getInstanceRaw()->save(UserPaths::getAppSettingsFilePath())) {
      mRecentModel->clearDirty();
    }
  }

  QtWindow::hideEvent(hideEvent);
}

void QtStartScreen::closeEvent(QCloseEvent* closeEvent) {
  QtWindow::closeEvent(closeEvent);
}

void QtStartScreen::createRecentProjects(QHBoxLayout* layout) {
  auto* vBoxLayout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  layout->addLayout(vBoxLayout, 1);

  auto* recentProjectsLabel = new QLabel(QStringLiteral("Recent Projects: "), this);
  recentProjectsLabel->setObjectName(QStringLiteral("titleLabel"));
  vBoxLayout->addWidget(recentProjectsLabel);

  constexpr int VBoxLayoutSpacing = 20;
  vBoxLayout->addSpacing(VBoxLayoutSpacing);

  auto maxRecentProjectsCount = IApplicationSettings::getInstanceRaw()->getMaxRecentProjectsCount();
  auto recentProjects = IApplicationSettings::getInstanceRaw()->getRecentProjects();

  auto* viewList = new QListView;    // NOLINT(cppcoreguidelines-owning-memory)
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  mRecentModel = new element::model::RecentItemModel(recentProjects, maxRecentProjectsCount);
  connect(viewList, &QListView::clicked, mRecentModel, &element::model::RecentItemModel::clicked);

  viewList->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(viewList, &QListView::customContextMenuRequested, this, [viewList, this](const QPoint& point) {
    QMenu contextMenu(tr("Context menu"), viewList);

    QAction action1(tr("Delete"), viewList);
    QObject::connect(&action1, &QAction::triggered, viewList, [viewList, point, this](auto) {
      const auto index = viewList->indexAt(point);
      if(index.isValid()) {
        mRecentModel->removeItem(index.row());
      }
    });
    contextMenu.addAction(&action1);

    contextMenu.exec(viewList->mapToGlobal(point));
  });

  // Drap/Drop
  viewList->setDefaultDropAction(Qt::MoveAction);
  viewList->setDragDropMode(QAbstractItemView::InternalMove);
  viewList->setDragEnabled(true);
  viewList->setDropIndicatorShown(true);
  viewList->setMovement(QListView::Snap);
  viewList->setSelectionMode(QAbstractItemView::SingleSelection);
  // Icon size
  constexpr QSize IconSize {30, 30};
  viewList->setIconSize(IconSize);
  viewList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
  viewList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  viewList->setUniformItemSizes(true);
  // Model
  viewList->setModel(mRecentModel);
  vBoxLayout->addWidget(viewList, 1);

  vBoxLayout->addStretch();
}

void QtStartScreen::createVersionAndGithub(QHBoxLayout* layout) {
  auto* vBoxLayout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  layout->addLayout(vBoxLayout, 3);

  // Create a Version label
  auto* pVersionLabel = new QLabel(fmt::format("Version {}", Version::getApplicationVersion().toString()).c_str(), this);
  pVersionLabel->setObjectName(QStringLiteral("boldLabel"));
  vBoxLayout->addWidget(pVersionLabel);

  constexpr std::array<int, 3> BoxLayoutSpacing = {20, 35, 8};
  vBoxLayout->addSpacing(BoxLayoutSpacing[0]);

  // Create a GitHub button
  auto* githubButton = createButton(this, QStringLiteral("View on GitHub"), QStringLiteral("infoButton"), []() {
    QDesktopServices::openUrl(QUrl("github"_g, QUrl::TolerantMode));
  });
  githubButton->setIcon(QIcon("://startscreen/github_icon.png"));
  vBoxLayout->addWidget(githubButton);

  vBoxLayout->addSpacing(BoxLayoutSpacing[1]);
  vBoxLayout->addStretch();

  vBoxLayout->addWidget(createButton(
      this, QStringLiteral("New Project"), QStringLiteral("projectButton"), [this]() { emit openNewProjectDialog(); }));
  vBoxLayout->addSpacing(BoxLayoutSpacing[2]);
  vBoxLayout->addWidget(createButton(
      this, QStringLiteral("Open Project"), QStringLiteral("projectButton"), [this]() { emit openOpenProjectDialog(); }));
}

}    // namespace qt::window