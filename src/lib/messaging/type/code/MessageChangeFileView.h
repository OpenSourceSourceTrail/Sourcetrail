#pragma once
// internal
#include "CodeScrollParams.h"
#include "FilePath.h"
#include "Message.h"
#include "TabId.h"

class MessageChangeFileView final : public Message<MessageChangeFileView> {
public:
  enum FileState { FILE_MINIMIZED, FILE_SNIPPETS, FILE_MAXIMIZED };

  enum ViewMode { VIEW_LIST, VIEW_SINGLE, VIEW_CURRENT };

  MessageChangeFileView(const FilePath& filePath_,
                        FileState state_,
                        ViewMode viewMode_,
                        CodeScrollParams scrollParams_,
                        bool switchesViewMode_ = false)
      : filePath(filePath_), state(state_), viewMode(viewMode_), scrollParams(scrollParams_), switchesViewMode(switchesViewMode_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageChangeFileView";
  }

  void print(std::wostream& ostream) const override {
    ostream << filePath.wstr();

    switch(state) {
    case FILE_MINIMIZED:
      ostream << L", minimize";
      break;
    case FILE_SNIPPETS:
      ostream << L", snippets";
      break;
    case FILE_MAXIMIZED:
      ostream << L", maximize";
      break;
    }

    switch(viewMode) {
    case VIEW_LIST:
      ostream << L", list";
      break;
    case VIEW_SINGLE:
      ostream << L", single";
      break;
    case VIEW_CURRENT:
      ostream << L", current";
      break;
    }
  }

  const FilePath filePath;
  const FileState state;
  const ViewMode viewMode;
  const CodeScrollParams scrollParams;
  const bool switchesViewMode;
};