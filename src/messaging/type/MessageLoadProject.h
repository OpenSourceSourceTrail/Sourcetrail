#pragma once
// internal
#include "FilePath.h"
#include "Message.h"
#include "RefreshInfo.h"

class MessageLoadProject : public Message<MessageLoadProject> {
public:
  explicit MessageLoadProject(FilePath filePath,
                              bool settingsChanged_ = false,
                              RefreshMode refreshMode_ = REFRESH_NONE,
                              bool shallowIndexingRequested_ = false);

  static std::string getStaticType() {
    return "MessageLoadProject";
  }

  void print(std::wostream& ostream) const override;

  const FilePath projectSettingsFilePath;
  const bool settingsChanged;
  const RefreshMode refreshMode;
  const bool shallowIndexingRequested;
};