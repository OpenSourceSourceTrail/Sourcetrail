#include "QtUndoRedoView.h"

#include "QtViewWidgetWrapper.h"
#include "ResourcePaths.h"
#include "utilityQt.h"

QtUndoRedoView::QtUndoRedoView(ViewLayout* viewLayout) : UndoRedoView(viewLayout), m_widget{new QtUndoRedo} {}

QtUndoRedoView::~QtUndoRedoView() = default;

void QtUndoRedoView::createWidgetWrapper() {
  setWidgetWrapper(std::make_shared<QtViewWidgetWrapper>(m_widget));
}

void QtUndoRedoView::refreshView() {
  m_onQtThread([this]() {
    m_widget->setStyleSheet(
        utility::getStyleSheet(ResourcePaths::getGuiDirectoryPath().concatenate(L"undoredo_view/undoredo_view.css")).c_str());
  });
}

void QtUndoRedoView::setRedoButtonEnabled(bool enabled) {
  m_onQtThread([this, enabled]() { m_widget->setRedoButtonEnabled(enabled); });
}

void QtUndoRedoView::setUndoButtonEnabled(bool enabled) {
  m_onQtThread([this, enabled]() { m_widget->setUndoButtonEnabled(enabled); });
}

void QtUndoRedoView::updateHistory(const std::vector<SearchMatch>& searchMatches, size_t currentIndex) {
  m_onQtThread([this, searchMatches, currentIndex]() { m_widget->updateHistory(searchMatches, currentIndex); });
}
