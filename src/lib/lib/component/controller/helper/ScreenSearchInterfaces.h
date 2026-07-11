#pragma once
#include <string>

class ScreenSearchResponder;

class ScreenSearchSender {
public:
  virtual ~ScreenSearchSender() = default;

  virtual void foundMatches(ScreenSearchResponder* responder, size_t matchCount) = 0;

  virtual void addResponder(ScreenSearchResponder* responder) = 0;
  virtual void removeResponder(ScreenSearchResponder* responder) = 0;

  virtual void clearMatches() = 0;
};

class ScreenSearchResponder {
public:
  virtual ~ScreenSearchResponder() = default;

  [[nodiscard]] virtual std::string getName() const = 0;
  [[nodiscard]] virtual bool isVisible() const = 0;

  virtual void findMatches(ScreenSearchSender* sender, const std::wstring& query) = 0;
  virtual void activateMatch(size_t matchIndex) = 0;
  virtual void deactivateMatch(size_t matchIndex) = 0;
  virtual void clearMatches() = 0;
};
