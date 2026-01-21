#pragma once
#include <QString>

#include "view.h"
#include "ViewLayout.h"

enum class Role : uint8_t { User, Assistant };

class ChatView : public View {
public:
  explicit ChatView(ViewLayout* viewLayout) noexcept;
  ~ChatView() override;

  [[nodiscard]] std::string getName() const override;

  virtual void addMessage(const QString& text, Role role) = 0;
};
