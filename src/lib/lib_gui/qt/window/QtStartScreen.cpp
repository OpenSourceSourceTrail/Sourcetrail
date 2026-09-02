#include "qt/window/QtStartScreen.hpp"

#include <array>
#include <ranges>
#include <utility>

#include <fmt/format.h>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QUrl>

#include "app/paths/UserPaths.h"
#include "qt/element/model/RecentItemModel.hpp"
#include "qt/utility/utilityQt.h"
#include "RangesTo.hpp"
#include "settings/IApplicationSettings.hpp"
#include "utility/globalStrings.h"
#include "Version.h"

namespace {

QPushButton* createButton(qt::window::QtStartScreen* that,
                          const QString& buttonName,
                          const QString& objectName,
                          std::function<void()> onClickEvent) {
  auto* newProjectButton = new QPushButton(buttonName, that);    // NOLINT(cppcoreguidelines-owning-memory)
#ifdef Q_OS_MAC
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
  constexpr QMargins LayoutMargins{15, 170, 15, 0};
  layout->setContentsMargins(LayoutMargins);
  m_content->setLayout(layout);

  createVersionAndGithub(layout);

  constexpr auto LayoutSpacing = 50;
  layout->addSpacing(LayoutSpacing);

  createRecentProjects(layout);

  // Move the window to center of the parent window.
  if(auto* parent = parentWidget(); parent != nullptr) {
    const QSize size = sizeHint();
    move(parent->width() / 2 - size.width() / 2, parent->height() / 2 - size.height() / 2);
  }

  setStyleSheet(utility::getStyleSheet(":/data/gui/startscreen/startscreen.css"));
}

void QtStartScreen::hideEvent(QHideEvent* hideEvent) {
  if(mRecentModel != nullptr && mRecentModel->isDirty()) {
    auto updatedRecentProjects = mRecentModel->getRecentProjects() | std::views::transform(&element::model::RecentItem::path) |
        utility::toContainer<std::vector<std::filesystem::path>>();

    IApplicationSettings::getInstanceRaw()->setRecentProjects(updatedRecentProjects);
    if(IApplicationSettings::getInstanceRaw()->save(UserPaths::getAppSettingsFilePath())) {
      mRecentModel->clearDirty();
    }
  }

  QtWindow::hideEvent(hideEvent);
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
  mRecentModel = new element::model::RecentItemModel(recentProjects, maxRecentProjectsCount, viewList);
  connect(viewList, &QListView::clicked, mRecentModel, &element::model::RecentItemModel::clicked);

  viewList->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(viewList, &QListView::customContextMenuRequested, this, [viewList, this](const QPoint& point) {
    QMenu contextMenu(tr("Context menu"), viewList);
    contextMenu.addAction(tr("Delete"), this, [viewList, point, this] {
      const auto index = viewList->indexAt(point);
      if(index.isValid()) {
        mRecentModel->removeItem(index.row());
      }
    });
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
  constexpr QSize IconSize{30, 30};
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
  githubButton->setIcon(QIcon(":/data/gui/startscreen/github_icon.png"));
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