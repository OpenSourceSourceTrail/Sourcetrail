#include "LanguagePackageJavaStub.h"

#include "IndexerJavaStub.h"

std::vector<std::shared_ptr<IndexerBase>> LanguagePackageJavaStub::instantiateSupportedIndexers() const {
  return {
      std::make_shared<IndexerJavaStub>(),
  };
}
