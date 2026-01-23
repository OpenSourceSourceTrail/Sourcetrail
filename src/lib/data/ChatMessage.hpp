#pragma once
#include <compare>
#include <cstdint>

#include <QDateTime>
#include <QString>

enum class MessageRole : std::uint8_t { User, Assistant, System, Error };

/**
 * @brief Immutable value object representing a chat message
 *
 * This class follows value semantics and is designed for use in Qt's
 * model/view architecture. All fields are const to enforce immutability.
 */
class ChatMessage final {
public:
  ChatMessage(QString content, MessageRole role, QDateTime timestamp = QDateTime::currentDateTime()) noexcept
      : mContent{std::move(content)}, mRole{role}, mTimestamp{std::move(timestamp)} {}

  [[nodiscard]] const QString& content() const noexcept {
    return mContent;
  }
  [[nodiscard]] MessageRole role() const noexcept {
    return mRole;
  }
  [[nodiscard]] const QDateTime& timestamp() const noexcept {
    return mTimestamp;
  }

  [[nodiscard]] auto operator<=>(const ChatMessage&) const noexcept = default;

private:
  QString mContent;
  MessageRole mRole;
  QDateTime mTimestamp;
};
