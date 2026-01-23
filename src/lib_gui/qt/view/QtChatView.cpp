#include "QtChatView.hpp"

#include <memory>

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "ChatModel.hpp"
#include "ChatView.hpp"
#include "MessageBubbleWidget.hpp"
#include "QtViewWidgetWrapper.h"


QtChatView::QtChatView(ViewLayout* viewLayout, std::shared_ptr<ChatModel> model)
    : ChatView{viewLayout}, mModel{std::move(model)}, mWidget{std::make_unique<QWidget>()} {
  setWidgetWrapper(std::make_shared<QtViewWidgetWrapper>(mWidget.get()));
  setupUI();
  setupConnections();
  loadStyleSheet();
}

QtChatView::~QtChatView() = default;

void QtChatView::setInputEnabled(bool enabled) {
  if(mInputField) {
    mInputField->setEnabled(enabled);
  }
}

void QtChatView::clearInput() {
  if(mInputField) {
    mInputField->clear();
  }
}

void QtChatView::focusInput() {
  if(mInputField) {
    mInputField->setFocus();
  }
}

void QtChatView::setupUI() {
  auto* mainLayout = new QVBoxLayout{mWidget.get()};
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  mainLayout->addWidget(createHeader());
  mainLayout->addWidget(createChatArea(), 1);
  mainLayout->addWidget(createInputArea());
}

void QtChatView::setupConnections() {
  // Model -> View data binding
  connect(mModel.get(), &ChatModel::messageAdded, this, &QtChatView::onMessageAdded);
  connect(mModel.get(), &ChatModel::messagesCleared, this, &QtChatView::onMessagesCleared);
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
  auto* header = new QFrame{mWidget.get()};
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

  connect(clearBtn, &QPushButton::clicked, this, &ChatView::clearRequested);

  layout->addWidget(titleLabel);
  layout->addStretch();
  layout->addWidget(clearBtn);

  return header;
}

QWidget* QtChatView::createChatArea() {
  mScrollArea = new QScrollArea{mWidget.get()};
  mScrollArea->setWidgetResizable(true);
  mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  mScrollArea->setFrameShape(QFrame::NoFrame);

  auto* chatWidget = new QWidget;
  mChatLayout = new QVBoxLayout{chatWidget};
  mChatLayout->setContentsMargins(12, 12, 12, 12);
  mChatLayout->setSpacing(12);
  mChatLayout->addStretch();

  mScrollArea->setWidget(chatWidget);

  return mScrollArea;
}

QWidget* QtChatView::createInputArea() {
  auto* inputWidget = new QFrame{mWidget.get()};
  inputWidget->setObjectName("inputArea");

  auto* layout = new QHBoxLayout{inputWidget};
  layout->setContentsMargins(12, 8, 12, 12);
  layout->setSpacing(8);

  mInputField = new QLineEdit{inputWidget};
  mInputField->setPlaceholderText("Ask Copilot a question...");
  mInputField->setMinimumHeight(36);

  connect(mInputField, &QLineEdit::returnPressed, this, &QtChatView::handleSubmit);

  auto* sendBtn = new QPushButton{"Send", inputWidget};
  sendBtn->setObjectName("sendButton");

  connect(sendBtn, &QPushButton::clicked, this, &QtChatView::handleSubmit);

  layout->addWidget(mInputField);
  layout->addWidget(sendBtn);

  return inputWidget;
}

void QtChatView::onMessageAdded(const ChatMessage& message) {
  // Remove stretch before adding new message
  if(mChatLayout->count() > 0) {
    auto* lastItem = mChatLayout->takeAt(mChatLayout->count() - 1);
    delete lastItem;
  }

  // Add message bubble (owned by Qt parent-child)
  auto* bubble = new MessageBubbleWidget{message, mChatLayout->parentWidget()};
  mChatLayout->addWidget(bubble);
  mChatLayout->addStretch();

  scrollToBottom();
}

void QtChatView::onMessagesCleared() {
  // Clear all widgets except stretch
  while(mChatLayout->count() > 0) {
    auto* item = mChatLayout->takeAt(0);
    if(auto* widget = item->widget()) {
      widget->deleteLater();
    }
    delete item;
  }
  mChatLayout->addStretch();
}

void QtChatView::scrollToBottom() {
  if(!mScrollArea) {
    return;
  }

  // Use event loop to ensure layout is updated
  QMetaObject::invokeMethod(
      this,
      [this]() {
        if(auto* scrollBar = mScrollArea->verticalScrollBar()) {
          scrollBar->setValue(scrollBar->maximum());
        }
      },
      Qt::QueuedConnection);
}

void QtChatView::handleSubmit() {
  QString text = mInputField->text().trimmed();
  if(text.isEmpty()) {
    return;
  }
  qDebug() << "User submitted message:" << text;

  // Forward to controller - view has no business logic
  emit ChatView::messageSubmitted(text);
}
