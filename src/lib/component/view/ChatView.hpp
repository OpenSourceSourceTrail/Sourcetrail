#pragma once
#include "view.h"
#include "ViewLayout.h"

class ChatView : public View {
public:
  explicit ChatView(ViewLayout* viewLayout) noexcept;
  ~ChatView() override;

  [[nodiscard]] std::string getName() const override;

  // virtual void sendMessage(const std::string& message) = 0;

  // virtual void clearChat() = 0;
};
