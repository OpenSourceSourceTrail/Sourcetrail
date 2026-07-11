#pragma once
#include <gmock/gmock.h>

#include "IProject.hpp"

struct MockedProject : IProject {
  ~MockedProject() override = default;
  MOCK_METHOD(FilePath, getProjectSettingsFilePath, (), (const, override));
  MOCK_METHOD(std::string, getDescription, (), (const, override));

  MOCK_METHOD(bool, isLoaded, (), (const, override));
  MOCK_METHOD(bool, isIndexing, (), (const, override));

  MOCK_METHOD(bool, settingsEqualExceptNameAndLocation, (const ProjectSettings&), (const, override));
  MOCK_METHOD(void, setStateOutdated, (), (override));

  MOCK_METHOD(void, load, (const std::shared_ptr<DialogView>&), (override));

  MOCK_METHOD(void, refresh, (std::shared_ptr<DialogView>, RefreshMode, bool), (override));

  MOCK_METHOD(RefreshInfo, getRefreshInfo, (RefreshMode), (const, override));

  MOCK_METHOD(void, buildIndex, (RefreshInfo, std::shared_ptr<DialogView>), (override));
};