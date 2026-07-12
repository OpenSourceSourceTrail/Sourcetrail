#pragma once

#include <vector>

#include "View.h"

class UndoRedoController;
struct SearchMatch;

class UndoRedoView : public View {
public:
  UndoRedoView(ViewLayout* viewLayout);
  ~UndoRedoView() override;

  std::string getName() const override;

  virtual void setRedoButtonEnabled(bool enabled) = 0;
  virtual void setUndoButtonEnabled(bool enabled) = 0;

  virtual void updateHistory(const std::vector<SearchMatch>& searchMatches, size_t currentIndex) = 0;

protected:
  UndoRedoController* getController();
};
