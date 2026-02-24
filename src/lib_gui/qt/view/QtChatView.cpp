#include "QtChatView.hpp"

#include <memory>

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>
#include <qlogging.h>

#include "ChatModel.hpp"
#include "ChatView.hpp"
#include "MessageBubbleDelegate.hpp"
#include "QtViewWidgetWrapper.h"


QtChatView::QtChatView(ViewLayout* viewLayout, std::shared_ptr<ChatModel> model)
    : ChatView{viewLayout}, mModel{std::move(model)}, mWidget{new QWidget} {
  setupUI();
  setupConnections();
  loadStyleSheet();
}

QtChatView::~QtChatView() = default;

void QtChatView::createWidgetWrapper() {
  setWidgetWrapper(std::make_shared<QtViewWidgetWrapper>(mWidget));
}

void QtChatView::setInputEnabled(bool enabled) {
  if(mInputField != nullptr) {
    mInputField->setEnabled(enabled);
  }
}

void QtChatView::clearInput() {
  if(mInputField != nullptr) {
    mInputField->clear();
  }
}

void QtChatView::focusInput() {
  if(mInputField != nullptr) {
    mInputField->setFocus();
  }
}

void QtChatView::setupUI() {
  auto* mainLayout = new QVBoxLayout{mWidget};
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  mainLayout->addWidget(createHeader());
  mainLayout->addWidget(createChatArea(), 1);
  mainLayout->addWidget(createInputArea());
}

void QtChatView::setupConnections() {
  // Connect model signals for scroll behavior
  [[maybe_unused]] auto conn1 = connect(mModel.get(), &ChatModel::messageAdded, this, &QtChatView::onMessageAdded);
  [[maybe_unused]] auto conn2 = connect(mModel.get(), &ChatModel::messagesCleared, this, &QtChatView::onMessagesCleared);
}

void QtChatView::loadStyleSheet() {
  // In production: load from external QSS file
  // For now: embedded for demonstration
  mWidget->setStyleSheet(R"(
        QWidget {
            background-color: #1E1E1E;
        }
        
        QFrame#header {
            background-color: #2D2D30;
            border-bottom: 1px solid #3E3E42;
        }
        
        QFrame#inputArea {
            background-color: #2D2D30;
            border-top: 1px solid #3E3E42;
        }
        
        QLineEdit {
            background-color: #3C3C3C;
            color: #E3E3E3;
            border: 1px solid #3E3E42;
            border-radius: 4px;
            padding: 8px;
            font-size: 13px;
        }
        
        QLineEdit:focus {
            border: 1px solid #007ACC;
        }
        
        QPushButton#sendButton {
            background-color: #007ACC;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 12px;
            font-weight: bold;
            min-width: 80px;
        }
        
        QPushButton#sendButton:hover {
            background-color: #005A9E;
        }
        
        QPushButton#sendButton:pressed {
            background-color: #004578;
        }
        
        QPushButton#clearButton {
            background-color: #3E3E42;
            color: #CCCCCC;
            border: none;
            border-radius: 3px;
            padding: 4px 8px;
            font-size: 11px;
            max-width: 60px;
        }
        
        QPushButton#clearButton:hover {
            background-color: #505052;
        }
        
        QScrollArea {
            background-color: #1E1E1E;
            border: none;
        }
        
        QScrollBar:vertical {
            background-color: #1E1E1E;
            width: 10px;
            margin: 0;
        }
        
        QScrollBar::handle:vertical {
            background-color: #3E3E42;
            border-radius: 5px;
            min-height: 20px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #505052;
        }
        
        MessageBubbleWidget[role="user"] {
            background-color: #007ACC;
            border-radius: 8px;
        }
        
        MessageBubbleWidget[role="user"] QLabel#contentLabel {
            color: white;
        }
        
        MessageBubbleWidget[role="assistant"] {
            background-color: #2D2D30;
            border-radius: 8px;
        }
        
        MessageBubbleWidget[role="assistant"] QLabel#contentLabel {
            color: #E3E3E3;
        }
        
        MessageBubbleWidget[role="error"] {
            background-color: #5A1D1D;
            border-radius: 8px;
            border: 1px solid #F48771;
        }
        
        MessageBubbleWidget[role="error"] QLabel#contentLabel {
            color: #F48771;
        }
        
        QLabel#timestampLabel {
            color: #858585;
        }
    )");
}

QWidget* QtChatView::createHeader() {
  auto* header = new QFrame{mWidget};
  header->setObjectName("header");

  auto* layout = new QHBoxLayout{header};
  layout->setContentsMargins(12, 8, 12, 8);

  auto* titleLabel = new QLabel{"LLM Chat", header};
  QFont font = titleLabel->font();
  font.setBold(true);
  font.setPointSize(11);
  titleLabel->setFont(font);
  titleLabel->setStyleSheet("color: #E3E3E3;");

  auto* clearBtn = new QPushButton{"Clear", header};
  clearBtn->setObjectName("clearButton");

  [[maybe_unused]] auto clearConn = connect(clearBtn, &QPushButton::clicked, this, &ChatView::clearRequested);

  layout->addWidget(titleLabel);
  layout->addStretch();
  layout->addWidget(clearBtn);

  return header;
}

QWidget* QtChatView::createChatArea() {
  mChatListView = new QListView{mWidget};
  mChatListView->setModel(mModel.get());
  mChatListView->setItemDelegate(new MessageBubbleDelegate{mChatListView});
  mChatListView->setObjectName("chatListView");
  mChatListView->setFrameShape(QFrame::NoFrame);
  mChatListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  mChatListView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  mChatListView->setSpacing(12);
  mChatListView->setUniformItemSizes(false);

  return mChatListView;
}

QWidget* QtChatView::createInputArea() {
  auto* inputWidget = new QFrame{mWidget};
  inputWidget->setObjectName("inputArea");

  auto* layout = new QHBoxLayout{inputWidget};
  layout->setContentsMargins(12, 8, 12, 12);
  layout->setSpacing(8);

  mInputField = new QLineEdit{inputWidget};
  mInputField->setPlaceholderText("Ask Copilot a question...");
  mInputField->setMinimumHeight(36);

  [[maybe_unused]] auto inputConn = connect(mInputField, &QLineEdit::returnPressed, this, &QtChatView::handleSubmit);

  auto* sendBtn = new QPushButton{"Send", inputWidget};
  sendBtn->setObjectName("sendButton");

  [[maybe_unused]] auto sendConn = connect(sendBtn, &QPushButton::clicked, this, &QtChatView::handleSubmit);

  layout->addWidget(mInputField);
  layout->addWidget(sendBtn);

  return inputWidget;
}

void QtChatView::onMessageAdded([[maybe_unused]] const ChatMessage& message) {
  // Scroll to bottom when new message is added
  if(mChatListView != nullptr) {
    const int lastRow = mModel->rowCount() - 1;
    if(lastRow >= 0) {
      mChatListView->scrollTo(mModel->index(lastRow), QAbstractItemView::EnsureVisible);
    }
  }
}

void QtChatView::onMessagesCleared() {
  // QListView automatically refreshes when model is cleared
  if(mChatListView != nullptr) {
    mChatListView->scrollToTop();
  }
}

void QtChatView::scrollToBottom() {
  if(mChatListView == nullptr) {
    return;
  }

  // Use event loop to ensure layout is updated
  QMetaObject::invokeMethod(
      this,
      [this]() {
        const int lastRow = mModel->rowCount() - 1;
        if(lastRow >= 0) {
          mChatListView->scrollTo(mModel->index(lastRow), QAbstractItemView::EnsureVisible);
        }
      },
      Qt::QueuedConnection);
}

void QtChatView::handleSubmit() {
  QString text = mInputField->text().trimmed();
  if(text.isEmpty()) {
    return;
  }

  // Forward to controller - view has no business logic
  emit ChatView::messageSubmitted(text);
}
