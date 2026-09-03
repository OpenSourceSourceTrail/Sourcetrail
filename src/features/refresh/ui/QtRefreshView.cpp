#include "refresh/ui/QtRefreshView.h"

#include <QCoreApplication>
#include <QFrame>
#include <QHBoxLayout>

#include "app/paths/ResourcePaths.h"
#include "indexing/messages/MessageIndexingShowDialog.h"
#include "qt/utility/utilityQt.h"
#include "qt/view/QtViewWidgetWrapper.h"
#include "refresh/messages/MessageRefresh.h"
#include "search/ui/QtSearchBarButton.h"

QtRefreshView::QtRefreshView(ViewLayout* viewLayout) : RefreshView(viewLayout), m_widget{new QFrame} {
  m_widget->setObjectName(QStringLiteral("refresh_bar"));

  QBoxLayout* layout = new QHBoxLayout;
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setAlignment(Qt::AlignTop);

  auto* refreshButton = new QtSearchBarButton{
      ResourcePaths::getGuiDirectoryPath().concatenate(L"refresh_view/images/refresh.png")};
  refreshButton->setObjectName(QStringLiteral("refresh_button"));
  refreshButton->setToolTip(QStringLiteral("refresh"));
  std::ignore = QObject::connect(refreshButton, &QPushButton::clicked, QCoreApplication::instance(), []() {
    MessageIndexingShowDialog().dispatch();
    MessageRefresh().dispatch();
  });

  layout->addWidget(refreshButton);
  m_widget->setLayout(layout);
}

QtRefreshView::~QtRefreshView() = default;

void QtRefreshView::createWidgetWrapper() {
  setWidgetWrapper(std::make_shared<QtViewWidgetWrapper>(m_widget));
}

void QtRefreshView::refreshView() {
  m_onQtThread([this]() {
    m_widget->setStyleSheet(
        utility::getStyleSheet(ResourcePaths::getGuiDirectoryPath().concatenate(L"refresh_view/refresh_view.css")).c_str());
  });
}
