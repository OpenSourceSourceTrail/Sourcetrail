#include "MessageLoadProject.h"

MessageLoadProject::MessageLoadProject(FilePath filePath,
                                       bool settingsChanged_,
                                       RefreshMode refreshMode_,
                                       bool shallowIndexingRequested_)
    : projectSettingsFilePath(std::move(filePath))
    , settingsChanged(settingsChanged_)
    , refreshMode(refreshMode_)
    , shallowIndexingRequested(shallowIndexingRequested_) {}

void MessageLoadProject::print(std::wostream& ostream) const {
  ostream << projectSettingsFilePath.wstr();
  ostream << L", settingsChanged: " << std::boolalpha << settingsChanged;
  ostream << L", refreshMode: " << refreshMode;
}