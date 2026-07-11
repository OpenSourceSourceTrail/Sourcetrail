#pragma once
#include "FileRegister.h"

class TestFileRegister final : public FileRegister {
public:
  TestFileRegister();
  ~TestFileRegister() override;
  bool hasFilePath(const FilePath& filePath) const override;
};