/**
 * @file CxxParserStdTestHelper.hpp
 * @author Ahmed Abdelaal (eng.ahmedhussein89@gmail.com)
 * @brief Shared fixture and parse helper for the per-standard CxxParser test suites
 * @version 0.1
 * @date 2026-08-26
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "data/indexer/IndexerStateInfo.h"
#include "data/parser/cxx/CxxParser.h"
#include "data/parser/ParserClientImpl.h"
#include "data/storage/IntermediateStorage.h"
#include "MockedApplicationSetting.hpp"
#include "TestFileRegister.h"
#include "TestStorage.h"
#include "TextAccess.h"
#include "utility.h"

namespace cxx_test {

/**
 * @brief Parse a snippet as `temp.cpp` with the given `-std=` flag and return the indexed symbols
 */
inline std::shared_ptr<TestStorage> parseCode(const std::string& code,
                                              const std::wstring& stdFlag,
                                              const std::vector<std::wstring>& compilerFlags = {}) {
  auto storage = std::make_shared<IntermediateStorage>();

  CxxParser parser(std::make_shared<ParserClientImpl>(storage.get()),
                   std::make_shared<TestFileRegister>(),
                   std::make_shared<IndexerStateInfo>());

  parser.buildIndex(
      L"temp.cpp", TextAccess::createFromString(code), utility::concat(compilerFlags, std::vector<std::wstring>(1, stdFlag)));

  return TestStorage::create(storage);
}

struct CxxParserStdTest : testing::Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mMocked);
    EXPECT_CALL(*mMocked, getLoggingEnabled).WillRepeatedly(testing::Return(false));
  }

  void TearDown() override {
    IApplicationSettings::setInstance(nullptr);
  }

  std::shared_ptr<MockedApplicationSettings> mMocked = std::make_shared<MockedApplicationSettings>();
};

}    // namespace cxx_test
