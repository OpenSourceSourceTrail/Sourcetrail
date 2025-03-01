#include "QtStatusView.h"

#include <tuple>

#include <QBoxLayout>
#include <QCheckBox>
#include <QFrame>
#include <QPushButton>
#include <QStandardItemModel>

#include "ColorScheme.h"
#include "IApplicationSettings.hpp"
#include "QtTable.h"
#include "QtViewWidgetWrapper.h"
#include "Status.h"
#include "to_underlying.hpp"
#include "type/MessageClearStatusView.h"
#include "type/MessageStatusFilterChanged.h"
#include "utilityQt.h"

QtStatusView::QtStatusView(ViewLayout* viewLayout) : StatusView(viewLayout) {
  setWidgetWrapper(std::make_shared<QtViewWidgetWrapper>(new QFrame));

  QWidget* widget = QtViewWidgetWrapper::getWidgetOfView(this);

  QBoxLayout* layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  widget->setLayout(layout);

  m_table = new QtTable{this};
  m_model = new QStandardItemModel{this};
  m_table->setModel(m_model);

  m_model->setColumnCount(2);
  m_table->setColumnWidth(STATUSVIEW_COLUMN::TYPE, 100);
  // m_table->setColumnWidth(STATUSVIEW_COLUMN::STATUS, 150);

  QStringList headers;
  headers << QStringLiteral("Type") << QStringLiteral("Message");
  m_model->setHorizontalHeaderLabels(headers);

  layout->addWidget(m_table);

  // Setup filters
  auto* filters = new QHBoxLayout;
  filters->setContentsMargins(10, 3, 0, 3);
  filters->setSpacing(25);

  const auto filter = static_cast<StatusFilter>(IApplicationSettings::getInstanceRaw()->getStatusFilter());
  m_showInfo = createFilterCheckbox(QStringLiteral("Info"), filters, filter & utility::to_underlying(StatusType::Info));
  m_showErrors = createFilterCheckbox(QStringLiteral("Error"), filters, filter & utility::to_underlying(StatusType::Error));

  filters->addStretch();

  auto* clearButton = new QPushButton(QStringLiteral("Clear Table"));
  clearButton->setObjectName(QStringLiteral("screen_button"));
  std::ignore = connect(clearButton, &QPushButton::clicked, this, []() { MessageClearStatusView().dispatch(); });

  filters->addWidget(clearButton);
  filters->addSpacing(10);

  layout->addLayout(filters);
}

QtStatusView::~QtStatusView() = default;

void QtStatusView::createWidgetWrapper() {}

void QtStatusView::refreshView() {
  m_onQtThread([this]() {
    QWidget* widget = QtViewWidgetWrapper::getWidgetOfView(this);
    utility::setWidgetBackgroundColor(widget, ColorScheme::getInstance()->getColor("window/background"));

    QPalette palette(m_showErrors->palette());
    palette.setColor(QPalette::WindowText, QColor(ColorScheme::getInstance()->getColor("table/text/normal").c_str()));

    m_showErrors->setPalette(palette);
    m_showInfo->setPalette(palette);

    m_table->updateRows();
  });
}

void QtStatusView::clear() {
  m_onQtThread([this]() {
    if(!m_model->index(0, 0).data(Qt::DisplayRole).toString().isEmpty()) {
      m_model->removeRows(0, m_model->rowCount());
    }

    m_table->showFirstRow();

    m_status.clear();
  });
}

void QtStatusView::addStatus(const std::vector<Status>& listOfStatus) {
  m_onQtThread([this, listOfStatus]() {
    for(const Status& status : listOfStatus) {
      const int rowNumber = m_table->getFilledRowCount();
      if(rowNumber < m_model->rowCount()) {
        m_model->insertRow(rowNumber);
      }

      QString statusType = (StatusType::Error == status.type ? QStringLiteral("ERROR") : QStringLiteral("INFO"));
      m_model->setItem(rowNumber, STATUSVIEW_COLUMN::TYPE, new QStandardItem(statusType));
      m_model->setItem(rowNumber, STATUSVIEW_COLUMN::STATUS, new QStandardItem(QString::fromStdWString(status.message)));

      if(StatusType::Error == status.type) {
        m_model->item(rowNumber, STATUSVIEW_COLUMN::TYPE)->setForeground(QBrush(Qt::red));
      }
    }

    m_table->updateRows();

    if(!m_table->hasSelection()) {
      m_table->showLastRow();
    }
  });
}

QCheckBox* QtStatusView::createFilterCheckbox(const QString& name, QBoxLayout* layout, bool checked) {
  auto* checkbox = new QCheckBox{name};
  checkbox->setChecked(checked);

  std::ignore = connect(checkbox, &QCheckBox::stateChanged, this, [this](int) {
    m_table->selectionModel()->clearSelection();

    const StatusFilter statusMask = (m_showInfo->isChecked() ? utility::to_underlying(StatusType::Info) :
                                                               utility::to_underlying(StatusType::None)) |
        (m_showErrors->isChecked() ? utility::to_underlying(StatusType::Error) : utility::to_underlying(StatusType::None));

    MessageStatusFilterChanged(statusMask).dispatch();
  });

  layout->addWidget(checkbox);

  return checkbox;
}
