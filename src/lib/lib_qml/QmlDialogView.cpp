#include "QmlDialogView.h"

#include <QString>

#include "AppShell.h"
#include "FilePath.h"
#include "utilityString.h"

namespace {

QString toQt(const std::wstring& text) {
  return QString::fromStdString(utility::encodeToUtf8(text));
}

int percentOf(size_t done, size_t total) {
  if(total == 0) {
    return 0;
  }
  return static_cast<int>((done * 100U) / total);
}

}    // namespace

QmlDialogView::QmlDialogView(UseCase useCase, StorageAccess* storageAccess, AppShell* shell)
    : DialogView(useCase, storageAccess), mShell(shell) {}

QmlDialogView::~QmlDialogView() = default;

void QmlDialogView::clearDialogs() {
  mShell->clearProgress();
}

void QmlDialogView::showUnknownProgressDialog(const std::wstring& title, const std::wstring& message) {
  // No total to divide by, so the bar reports -1 and QML shows it indeterminate.
  mShell->reportProgress(toQt(title) + QStringLiteral(" - ") + toQt(message), -1);
}

void QmlDialogView::hideUnknownProgressDialog() {
  mShell->clearProgress();
}

void QmlDialogView::showProgressDialog(const std::wstring& title, const std::wstring& message, size_t progress) {
  mShell->reportProgress(toQt(title) + QStringLiteral(" - ") + toQt(message), static_cast<int>(progress));
}

void QmlDialogView::hideProgressDialog() {
  mShell->clearProgress();
}

void QmlDialogView::updateIndexingDialog(size_t /*startedFileCount*/,
                                         size_t finishedFileCount,
                                         size_t totalFileCount,
                                         const std::vector<FilePath>& sourcePaths) {
  const auto current = sourcePaths.empty() ? QString{} : QString::fromStdString(sourcePaths.back().str());
  mShell->reportProgress(QStringLiteral("Indexing %1/%2  %3").arg(finishedFileCount).arg(totalFileCount).arg(current),
                         percentOf(finishedFileCount, totalFileCount));
}

void QmlDialogView::updateCustomIndexingDialog(size_t startedFileCount,
                                               size_t finishedFileCount,
                                               size_t totalFileCount,
                                               const std::vector<FilePath>& sourcePaths) {
  updateIndexingDialog(startedFileCount, finishedFileCount, totalFileCount, sourcePaths);
}

DatabasePolicy QmlDialogView::finishedIndexingDialog(size_t /*indexedFileCount*/,
                                                     size_t /*totalIndexedFileCount*/,
                                                     size_t completedFileCount,
                                                     size_t totalFileCount,
                                                     float time,
                                                     ErrorCountInfo errorInfo,
                                                     bool /*interrupted*/,
                                                     bool /*shallow*/) {
  mShell->clearProgress();
  mShell->reportStatus(QStringLiteral("Indexed %1/%2 files in %3s, %4 errors")
                           .arg(completedFileCount)
                           .arg(totalFileCount)
                           .arg(static_cast<double>(time), 0, 'f', 1)
                           .arg(errorInfo.total),
                       errorInfo.total > 0);
  return DATABASE_POLICY_KEEP;
}
