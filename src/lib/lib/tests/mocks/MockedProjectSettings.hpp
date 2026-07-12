#pragma once
#include <gmock/gmock.h>

#include "ProjectSettings.h"

struct MockedProjectSettings : ProjectSettings {
  MOCK_METHOD(bool, load, (const FilePath&, bool), (override));
  MOCK_METHOD(bool, loadFromString, (const std::string&, bool), (override));
  MOCK_METHOD(bool, save, (), (override));
  MOCK_METHOD(bool, save, (const FilePath&), (override));
};