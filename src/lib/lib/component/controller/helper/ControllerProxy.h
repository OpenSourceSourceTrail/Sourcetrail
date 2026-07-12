#pragma once
// STL
#include <functional>
// internal
#include "../../../../scheduling/TaskLambda.h"
#include "View.h"

template <typename ControllerType>
class ControllerProxy {
public:
  ControllerProxy(View* pView, Id schedulerId) : m_pView(pView), m_schedulerId(schedulerId) {}

  void execute(std::function<void(ControllerType*)> callback) {
    ControllerType* pController = m_pView->getController<ControllerType>();
    if(pController != nullptr) {
      callback(pController);
    }
  }

  void executeAsTask(std::function<void(ControllerType*)> callback) {
    ControllerType* pController = m_pView->getController<ControllerType>();
    if(pController != nullptr) {
      Task::dispatch(m_schedulerId, std::make_shared<TaskLambda>(std::bind(callback, pController)));
    }
  }

  template <typename FuncType, typename... Args>
  void executeAsTaskWithArgs(FuncType callback, const Args... args) {
    ControllerType* pController = m_pView->getController<ControllerType>();
    if(pController != nullptr) {
      Task::dispatch(m_schedulerId, std::make_shared<TaskLambda>([func = std::bind(callback, pController, args...)]() { func(); }));
    }
  }

private:
  View* m_pView;
  Id m_schedulerId;
};