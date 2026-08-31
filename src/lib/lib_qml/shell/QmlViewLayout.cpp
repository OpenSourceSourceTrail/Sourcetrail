#include "shell/QmlViewLayout.h"

#include <utility>

#include "component/view/View.h"

namespace shell {

QmlViewLayout::QmlViewLayout(ShowHandler onShow) : mOnShow(std::move(onShow)) {}

QmlViewLayout::~QmlViewLayout() = default;

// The QML scene decides what exists and when; there is no registry of views to keep here.
void QmlViewLayout::addView(View* /*view*/) {}

void QmlViewLayout::removeView(View* /*view*/) {}

void QmlViewLayout::showView(View* view) {
  if(mOnShow && view != nullptr) {
    mOnShow(view->getName());
  }
}

// Nothing hides itself in this design -- a panel stays until the user picks another one.
void QmlViewLayout::hideView(View* /*view*/) {}

void QmlViewLayout::setViewEnabled(View* /*view*/, bool /*enabled*/) {}

}    // namespace shell
