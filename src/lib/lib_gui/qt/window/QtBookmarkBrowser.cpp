#include "QtBookmarkBrowser.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "QtBookmark.h"
#include "QtBookmarkCategory.h"
#include "utilityQt.h"

QtBookmarkBrowser::QtBookmarkBrowser(ControllerProxy<BookmarkController>* controllerProxy, QWidget* parent)
    : QtWindow{false, parent}, mControllerProxy{controllerProxy} {
  assert(controllerProxy != nullptr);
}

QtBookmarkBrowser::~QtBookmarkBrowser() = default;

void QtBookmarkBrowser::setupBookmarkBrowser() {
  setStyleSheet(utility::getStyleSheet("://window/window.css") + utility::getStyleSheet("://bookmark_view/bookmark_view.css"));

  mHeaderBackground = new QWidget(m_window);    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
  mHeaderBackground->setObjectName(QStringLiteral("header_background"));
  mHeaderBackground->setGeometry(0, 0, 0, 0);
  mHeaderBackground->lower();

  auto* layout = new QHBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setAlignment(Qt::AlignTop);
  setLayout(layout);

  {
    auto* headerLayout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    headerLayout->setSpacing(0);
    headerLayout->setContentsMargins(25, 35, 25, 35);
    headerLayout->setAlignment(Qt::AlignTop);
    layout->addLayout(headerLayout);

    headerLayout->addStrut(150);

    auto* title = new QLabel(QStringLiteral("Bookmarks"));    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    title->setObjectName(QStringLiteral("title"));
    headerLayout->addWidget(title);

    headerLayout->addSpacing(40);

    auto* filterLabel = new QLabel(QStringLiteral("Show:"));    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    filterLabel->setObjectName(QStringLiteral("filter_label"));
    headerLayout->addWidget(filterLabel);

    mFilterComboBox = new QComboBox();    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    mFilterComboBox->addItem(QStringLiteral("All"));
    mFilterComboBox->addItem(QStringLiteral("Nodes"));
    mFilterComboBox->addItem(QStringLiteral("Edges"));
    mFilterComboBox->setObjectName(QStringLiteral("filter_box"));
    headerLayout->addWidget(mFilterComboBox);

    std::ignore = connect(
        mFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QtBookmarkBrowser::filterOrOrderChanged);

    headerLayout->addSpacing(40);

    auto* orderLabel = new QLabel(QStringLiteral("Sort by:"));    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    orderLabel->setObjectName(QStringLiteral("order_label"));
    headerLayout->addWidget(orderLabel);

    mOrderNames.emplace_back("Name asc.");
    mOrderNames.emplace_back("Name des.");
    mOrderNames.emplace_back("Date asc.");
    mOrderNames.emplace_back("Date des.");

    mOrderComboBox = new QComboBox(this);    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    mOrderComboBox->addItem(mOrderNames[0].c_str());
    mOrderComboBox->addItem(mOrderNames[1].c_str());
    mOrderComboBox->addItem(mOrderNames[2].c_str());
    mOrderComboBox->addItem(mOrderNames[3].c_str());
    mOrderComboBox->setCurrentIndex(3);
    mOrderComboBox->setObjectName(QStringLiteral("order_box"));
    headerLayout->addWidget(mOrderComboBox);

    std::ignore = connect(
        mOrderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QtBookmarkBrowser::filterOrOrderChanged);
  }

  {
    auto* bodyLayout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    bodyLayout->setSpacing(0);
    bodyLayout->setContentsMargins(0, 10, 0, 10);
    bodyLayout->setAlignment(Qt::AlignLeft);
    layout->addLayout(bodyLayout);

    mBookmarkTree = new QTreeWidget;    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    mBookmarkTree->setObjectName(QStringLiteral("bookmark_tree"));
    mBookmarkTree->setAttribute(Qt::WA_MacShowFocusRect, false);
    mBookmarkTree->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    mBookmarkTree->header()->close();
    mBookmarkTree->setIndentation(0);
    mBookmarkTree->setHeaderLabel(QStringLiteral("Bookmarks"));

    std::ignore = connect(mBookmarkTree, &QTreeWidget::itemClicked, this, &QtBookmarkBrowser::treeItemClicked);

    bodyLayout->addWidget(mBookmarkTree);

    bodyLayout->addSpacing(15);

    QHBoxLayout* buttonLayout = createButtons();
    buttonLayout->setContentsMargins(0, 0, 23, 13);
    bodyLayout->addLayout(buttonLayout);
    setPreviousVisible(false);
    setCloseVisible(false);
    updateNextButton(QStringLiteral("Close"));
  }
}

void QtBookmarkBrowser::setBookmarks(const std::vector<std::shared_ptr<Bookmark>>& bookmarks) {
  std::map<QString, bool> wasCategoryExpanded;
  for(int i = 0; i < mBookmarkTree->topLevelItemCount(); i++) {
    QTreeWidgetItem* item = mBookmarkTree->topLevelItem(i);
    wasCategoryExpanded.emplace(item->whatsThis(0), item->isExpanded());
  }

  mBookmarkTree->clear();

  std::map<std::wstring, BookmarkCategory> categoryNamesOrdered;
  for(const std::shared_ptr<Bookmark>& bookmark : bookmarks) {
    categoryNamesOrdered.emplace(bookmark->getCategory().getName(), bookmark->getCategory());
  }

  for(const auto& [categoryName, type] : categoryNamesOrdered) {
    findOrCreateTreeCategory(type);
  }

  for(const std::shared_ptr<Bookmark>& bookmark : bookmarks) {
    auto* qtBookmark = new QtBookmark(mControllerProxy);    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    qtBookmark->setBookmark(bookmark);

    QTreeWidgetItem* categoryItem = findOrCreateTreeCategory(bookmark->getCategory());
    auto* treeWidgetItem = new QTreeWidgetItem(categoryItem);    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
    mBookmarkTree->setItemWidget(treeWidgetItem, 0, qtBookmark);
    categoryItem->addChild(treeWidgetItem);
    qtBookmark->setTreeWidgetItem(categoryItem);

    bool wasExpanded = true;
    auto iterator = wasCategoryExpanded.find(categoryItem->whatsThis(0));
    if(iterator != wasCategoryExpanded.end()) {
      wasExpanded = iterator->second;
    }

    categoryItem->setExpanded(!wasExpanded);
    categoryItem->setExpanded(wasExpanded);
  }
}

void QtBookmarkBrowser::resizeEvent(QResizeEvent* event) {
  QtWindow::resizeEvent(event);

  mHeaderBackground->setGeometry(0, 0, 200, m_window->size().height());
}

void QtBookmarkBrowser::handleClose() {
  close();
}

void QtBookmarkBrowser::handleNext() {
  close();
}

void QtBookmarkBrowser::filterOrOrderChanged(int /*index*/) {
  const Bookmark::Filter filter = getSelectedFilter();
  const Bookmark::Order order = getSelectedOrder();

  mControllerProxy->executeAsTaskWithArgs(&BookmarkController::displayBookmarksFor, filter, order);
}

void QtBookmarkBrowser::treeItemClicked(QTreeWidgetItem* item, int /*column*/) {
  auto* category = dynamic_cast<QtBookmarkCategory*>(mBookmarkTree->itemWidget(item, 0));
  if(category != nullptr) {
    category->expandClicked();
    return;
  }

  auto* bookmark = dynamic_cast<QtBookmark*>(mBookmarkTree->itemWidget(item, 0));
  if(bookmark != nullptr) {
    bookmark->commentToggled();
    return;
  }
}

Bookmark::Filter QtBookmarkBrowser::getSelectedFilter() {
  std::string text = mFilterComboBox->currentText().toStdString();

  if(text == "Nodes") {
    return Bookmark::Filter::Nodes;
  } else if(text == "Edges") {
    return Bookmark::Filter::Edges;
  }

  return Bookmark::Filter::All;
}

Bookmark::Order QtBookmarkBrowser::getSelectedOrder() {
  std::string orderString = mOrderComboBox->currentText().toStdString();

  if(orderString == mOrderNames[0]) {
    return Bookmark::Order::NameAscending;
  } else if(orderString == mOrderNames[1]) {
    return Bookmark::Order::NameDescending;
  } else if(orderString == mOrderNames[2]) {
    return Bookmark::Order::DateAscending;
  } else if(orderString == mOrderNames[3]) {
    return Bookmark::Order::DateDescending;
  } else {
    return Bookmark::Order::None;
  }
}

QTreeWidgetItem* QtBookmarkBrowser::findOrCreateTreeCategory(const BookmarkCategory& category) {
  for(int i = 0; i < mBookmarkTree->topLevelItemCount(); i++) {
    QTreeWidgetItem* item = mBookmarkTree->topLevelItem(i);

    if(item->whatsThis(0).toStdWString() == category.getName()) {
      return item;
    }
  }

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): Qt handles the memory
  auto* categoryItem = new QtBookmarkCategory{mControllerProxy};
  if(category.getName().length() > 0) {
    categoryItem->setName(category.getName());
  } else {
    categoryItem->setName(L"No Category");
  }
  categoryItem->setId(category.getId());

  auto* newItem = new QTreeWidgetItem(mBookmarkTree);    // NOLINT(cppcoreguidelines-owning-memory): Qt handles the memory
  newItem->setWhatsThis(0, QString::fromStdWString(category.getName()));

  categoryItem->setTreeWidgetItem(newItem);

  mBookmarkTree->setItemWidget(newItem, 0, categoryItem);
  mBookmarkTree->addTopLevelItem(newItem);

  return newItem;
}
