#pragma once

#include <vector>

#include "View.h"
#include "ViewLayout.h"

class TabbedView
    : public View
    , public ViewLayout {
public:
  TabbedView(ViewLayout* viewLayout, const std::string& name);
  ~TabbedView() override;

  [[nodiscard]] const std::vector<View*>& getViews() const;

  virtual void addViewWidget(View* view) = 0;

  // View implementation
  [[nodiscard]] std::string getName() const override;

  // ViewLayout implementation
  void addView(View* view) override;
  void removeView(View* view) override;

  void showView(View* view) override;
  void hideView(View* view) override;

  void setViewEnabled(View* view, bool enabled) override;

private:
  std::vector<View*> m_views;
  std::string m_name;
};
