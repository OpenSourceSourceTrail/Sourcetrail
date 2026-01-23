#include "ChatView.hpp"

ChatView::ChatView(ViewLayout* viewLayout) noexcept : View(viewLayout) {}

ChatView::~ChatView() = default;

std::string ChatView::getName() const {
  return "ChatView";
}

void ChatView::createWidgetWrapper() {}

void ChatView::refreshView() {}
