#pragma once

#include "View.h"

class RefreshController;

class RefreshView : public View {
public:
  explicit RefreshView(ViewLayout* viewLayout);
  ~RefreshView() override;

  [[nodiscard]] std::string getName() const override;

private:
  RefreshController* getController();
};
