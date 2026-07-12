#pragma once
#include <gmock/gmock.h>

#include "DialogView.h"

struct MockedDialogView : DialogView {
  explicit MockedDialogView() : DialogView(UseCase::INDEXING, nullptr) {}

  MOCK_METHOD(void, clearDialogs, (), (override));

  MOCK_METHOD(void, showUnknownProgressDialog, (const std::wstring&, const std::wstring&), (override));

  MOCK_METHOD(void, hideUnknownProgressDialog, (), (override));

  MOCK_METHOD(void, showProgressDialog, (const std::wstring&, const std::wstring&, size_t), (override));

  MOCK_METHOD(void, hideProgressDialog, (), (override));

  MOCK_METHOD(void,
              startIndexingDialog,
              (Project*,
               const std::vector<RefreshMode>&,
               const RefreshMode,
               bool,
               bool,
               std::function<void(const RefreshInfo& info)>,
               std::function<void()>),
              (override));

  MOCK_METHOD(void, updateIndexingDialog, (size_t, size_t, size_t, const std::vector<FilePath>&), (override));

  MOCK_METHOD(void, updateCustomIndexingDialog, (size_t, size_t, size_t, const std::vector<FilePath>&), (override));


  MOCK_METHOD(DatabasePolicy,
              finishedIndexingDialog,
              (size_t, size_t, size_t, size_t, float, ErrorCountInfo, bool, bool),
              (override));

  MOCK_METHOD(int, confirm, (const std::wstring&, const std::vector<std::wstring>&), (override));
};
