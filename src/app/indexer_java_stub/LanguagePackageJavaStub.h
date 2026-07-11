#pragma once

#include "LanguagePackage.h"

class LanguagePackageJavaStub : public LanguagePackage {
public:
  [[nodiscard]] std::vector<std::shared_ptr<IndexerBase>> instantiateSupportedIndexers() const override;
};
