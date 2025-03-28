#include "QtRefreshView.h"

#include <QCoreApplication>
#include <QFrame>
#include <QHBoxLayout>

#include "QtSearchBarButton.h"
#include "QtViewWidgetWrapper.h"
#include "ResourcePaths.h"
#include "type/indexing/MessageIndexingShowDialog.h"
#include "type/MessageRefresh.h"
#include "utilityQt.h"

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
