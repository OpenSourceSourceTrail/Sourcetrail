#pragma once

#include <set>

#include "BaseInterprocessDataManager.h"
#include "FilePath.h"

class InterprocessIndexingStatusManager : public BaseInterprocessDataManager {
public:
  InterprocessIndexingStatusManager(const std::string& instanceUuid, Id processId, bool isOwner);
  ~InterprocessIndexingStatusManager() override;

  void startIndexingSourceFile(const FilePath& filePath);
  void finishIndexingSourceFile();

  void setIndexingInterrupted(bool interrupted);
  bool getIndexingInterrupted();

  Id getNextFinishedProcessId();

  std::vector<FilePath> getCurrentlyIndexedSourceFilePaths();
  std::vector<FilePath> getCrashedSourceFilePaths();

private:
  static const char* sSharedMemoryNamePrefix;

  static const char* sIndexingFilesKeyName;
  static const char* sCurrentFilesKeyName;
  static const char* sCrashedFilesKeyName;
  static const char* sFinishedProcessIdsKeyName;
  static const char* sIndexingInterruptedKeyName;
};