#include "app/LanguagePackageManager.h"

#include "app/LanguagePackage.h"
#include "indexing/logic/IndexerComposite.h"

LanguagePackageManager::Ptr LanguagePackageManager::getInstance() {
  std::call_once(s_once, [] { s_instance = std::shared_ptr<LanguagePackageManager>(new LanguagePackageManager()); });
  return s_instance;
}

void LanguagePackageManager::destroyInstance() {
  s_instance.reset();
}

void LanguagePackageManager::addPackage(std::shared_ptr<LanguagePackage> package) {
  m_packages.push_back(std::move(package));
}

std::shared_ptr<IndexerComposite> LanguagePackageManager::instantiateSupportedIndexers() {
  auto composite = std::make_shared<IndexerComposite>();
  for(const auto& package : m_packages) {
    for(const auto& indexer : package->instantiateSupportedIndexers()) {
      composite->addIndexer(indexer);
    }
  }
  return composite;
}

LanguagePackageManager::Ptr LanguagePackageManager::s_instance;
std::once_flag LanguagePackageManager::s_once;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

LanguagePackageManager::LanguagePackageManager() = default;