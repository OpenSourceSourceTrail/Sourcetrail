#pragma once

#include <vector>

#include "View.h"

struct Status;

class StatusView : public View {
public:
  explicit StatusView(ViewLayout* viewLayout);
  ~StatusView() override;

  std::string getName() const override;

  virtual void addStatus(const std::vector<Status>& status) = 0;
  virtual void clear() = 0;
};
