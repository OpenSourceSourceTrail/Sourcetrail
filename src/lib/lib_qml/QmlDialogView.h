#pragma once
#include "component/view/DialogView.h"

class AppShell;

/**
 * Turns the indexing pipeline's progress reports into shell properties QML can bind to.
 *
 * Application hands one of these out per use case in place of the base DialogView, which silently
 * drops everything. finishedIndexingDialog answers KEEP straight away: the widget GUI asked the
 * user, but nothing in the QML shell has a keep/discard prompt yet, and the alternative -- blocking
 * the indexing thread on an answer that never comes -- is the bug the engine daemon already has.
 */
class QmlDialogView final : public DialogView {
public:
  QmlDialogView(UseCase useCase, StorageAccess* storageAccess, AppShell* shell);
  ~QmlDialogView() override;

  void clearDialogs() override;

  void showUnknownProgressDialog(const std::wstring& title, const std::wstring& message) override;
  void hideUnknownProgressDialog() override;

  void showProgressDialog(const std::wstring& title, const std::wstring& message, size_t progress) override;
  void hideProgressDialog() override;

  void updateIndexingDialog(size_t startedFileCount,
                            size_t finishedFileCount,
                            size_t totalFileCount,
                            const std::vector<FilePath>& sourcePaths) override;
  void updateCustomIndexingDialog(size_t startedFileCount,
                                  size_t finishedFileCount,
                                  size_t totalFileCount,
                                  const std::vector<FilePath>& sourcePaths) override;

  DatabasePolicy finishedIndexingDialog(size_t indexedFileCount,
                                        size_t totalIndexedFileCount,
                                        size_t completedFileCount,
                                        size_t totalFileCount,
                                        float time,
                                        ErrorCountInfo errorInfo,
                                        bool interrupted,
                                        bool shallow) override;

private:
  AppShell* mShell;
};
