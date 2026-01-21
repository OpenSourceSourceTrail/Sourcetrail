#include "MessageBubble.hpp"

#include <qboxlayout.h>
#include <qlabel.h>


MessageBubble::MessageBubble(const QString& text, Role role, QWidget* parent) : QFrame(parent), m_role(role) {
  setupUI(text);
}

void MessageBubble::setupUI(const QString& text) {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 8, 12, 8);

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto* label = new QLabel(text, this);
  label->setWordWrap(true);
  label->setStyleSheet("color: #E3E3E3;");

  layout->addWidget(label);

  setStyleSheet(R"(
            MessageBubble {
                background-color: #2D2D30;
                border-radius: 8px;
            }
        )");
}
