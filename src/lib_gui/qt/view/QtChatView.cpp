#include "QtChatView.hpp"

#include <memory>
#include <tuple>

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>

#include "ChatController.hpp"
#include "MessageBubble.hpp"
#include "QtViewWidgetWrapper.h"


QtChatView::QtChatView(ViewLayout* viewLayout) noexcept : ChatView{viewLayout}, m_widget{new QWidget} {
  setWidgetWrapper(std::make_shared<QtViewWidgetWrapper>(m_widget));
  setupUI();
  applyStyles();
}

QtChatView::~QtChatView() = default;


void QtChatView::sendMessage() {
  QString text = m_inputField->text().trimmed();
  if(text.isEmpty()) {
    return;
  }
  m_inputField->clear();

  getController<ChatController>()->sendMessage(text);
}

void QtChatView::clearChat() {
  // Remove all message bubbles
  while(auto* item = m_chatLayout->takeAt(0)) {
    if(auto* widget = item->widget()) {
      widget->deleteLater();
    }
    delete item;
  }
  m_chatLayout->addStretch();
}

void QtChatView::setupUI() {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* mainLayout = new QVBoxLayout{m_widget};
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Header
  auto* header = createHeader();
  mainLayout->addWidget(header);

  // Chat area
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* scrollArea = new QScrollArea(m_widget);
  scrollArea->setWidgetResizable(true);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setFrameShape(QFrame::NoFrame);

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* chatWidget = new QWidget;
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  m_chatLayout = new QVBoxLayout(chatWidget);
  m_chatLayout->setContentsMargins(12, 12, 12, 12);
  m_chatLayout->setSpacing(12);
  m_chatLayout->addStretch();

  scrollArea->setWidget(chatWidget);
  mainLayout->addWidget(scrollArea, 1);

  // Input area
  auto* inputArea = createInputArea();
  mainLayout->addWidget(inputArea);
}

QWidget* QtChatView::createHeader() {
  auto* header = new QFrame(m_widget);
  auto* layout = new QHBoxLayout(header);
  layout->setContentsMargins(12, 8, 12, 8);

  // Title with icon
  auto* titleLabel = new QLabel("✨ Copilot Chat", header);
  QFont font = titleLabel->font();
  font.setBold(true);
  font.setPointSize(11);
  titleLabel->setFont(font);
  titleLabel->setStyleSheet("color: #E3E3E3;");

  // Clear button
  auto* clearBtn = new QPushButton("Clear", header);
  clearBtn->setMaximumWidth(60);
  std::ignore = QObject::connect(clearBtn, &QPushButton::clicked, m_widget, [this]() { clearChat(); });

  layout->addWidget(titleLabel);
  layout->addStretch();
  layout->addWidget(clearBtn);

  header->setStyleSheet(R"(
            QFrame {
                background-color: #2D2D30;
                border-bottom: 1px solid #3E3E42;
            }
            QPushButton {
                background-color: #3E3E42;
                color: #CCCCCC;
                border: none;
                border-radius: 3px;
                padding: 4px 8px;
                font-size: 11px;
            }
            QPushButton:hover {
                background-color: #505052;
            }
        )");

  return header;
}

QWidget* QtChatView::createInputArea() {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* inputWidget = new QFrame(m_widget);
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* layout = new QHBoxLayout(inputWidget);
  layout->setContentsMargins(12, 8, 12, 12);
  layout->setSpacing(8);

  // Input field
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  m_inputField = new QLineEdit(inputWidget);
  m_inputField->setPlaceholderText("Ask Copilot a question...");
  m_inputField->setMinimumHeight(36);
  std::ignore = QObject::connect(m_inputField, &QLineEdit::returnPressed, m_widget, [this]() { sendMessage(); });

  // Button row
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* sendBtn = new QPushButton("Send", inputWidget);
  sendBtn->setMinimumWidth(80);
  std::ignore = QObject::connect(sendBtn, &QPushButton::clicked, m_widget, [this]() { sendMessage(); });

  layout->addWidget(m_inputField);
  layout->addWidget(sendBtn);

  inputWidget->setStyleSheet(R"(
            QFrame {
                background-color: #2D2D30;
                border-top: 1px solid #3E3E42;
            }
        )");

  return inputWidget;
}

void QtChatView::addMessage(const QString& text, Role role) {
  // Remove stretch before adding new message
  if(m_chatLayout->count() > 0) {
    auto* item = m_chatLayout->takeAt(m_chatLayout->count() - 1);
    delete item;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* bubble = new MessageBubble(text, role, m_widget);
  m_chatLayout->addWidget(bubble);
  m_chatLayout->addStretch();

  // Scroll to bottom
  QTimer::singleShot(50, m_widget, [this]() {
    if(auto* scrollArea = qobject_cast<QScrollArea*>(m_chatLayout->parentWidget()->parentWidget())) {
      scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    }
  });
}

QString QtChatView::generateResponse(const QString& query) const {
  // Simple mock responses using C++20 features
  static const std::vector<std::pair<QString, QString>> responses = {
      {"hello", "Hello! How can I assist you with your code today?"},
      {"help", "I can help you with code explanations, debugging, refactoring, and answering programming questions."},
      {"explain", "Sure! Please share the code you'd like me to explain."},
      {"default", "I'm here to help! Could you provide more details about what you need?"}};

  auto lower = query.toLower();

  // C++20 ranges to find matching response
  auto it = std::ranges::find_if(responses, [&lower](const auto& pair) { return lower.contains(pair.first); });

  return it != responses.end() ? it->second : responses.back().second;
}

void QtChatView::applyStyles() {
  m_widget->setStyleSheet(R"(
            CopilotChatWidget {
                background-color: #1E1E1E;
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
            QPushButton {
                background-color: #007ACC;
                color: white;
                border: none;
                border-radius: 4px;
                padding: 8px 16px;
                font-size: 12px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #005A9E;
            }
            QPushButton:pressed {
                background-color: #004578;
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
        )");
}
