#include "project/logic/SourceGroupFactory.h"

#include "logging.h"
#include "project/logic/SourceGroup.h"
#include "project/logic/SourceGroupFactoryModule.h"
#include "settings/source_group/SourceGroupSettings.h"

std::shared_ptr<SourceGroupFactory> SourceGroupFactory::getInstance() {
  std::call_once(s_once, [] { s_instance = std::shared_ptr<SourceGroupFactory>(new SourceGroupFactory()); });
  return s_instance;
}

void SourceGroupFactory::addModule(std::shared_ptr<SourceGroupFactoryModule> module) {
  m_modules.push_back(module);
}

std::vector<std::shared_ptr<SourceGroup>> SourceGroupFactory::createSourceGroups(
    std::vector<std::shared_ptr<SourceGroupSettings>> allSourceGroupSettings) {
  std::vector<std::shared_ptr<SourceGroup>> sourceGroups;
  for(const std::shared_ptr<SourceGroupSettings>& sourceGroupSettings : allSourceGroupSettings) {
    std::shared_ptr<SourceGroup> sourceGroup = createSourceGroup(sourceGroupSettings);
    if(sourceGroup) {
      sourceGroups.push_back(sourceGroup);
    } else {
      LOG_WARNING("Skipping source group of type \"" + sourceGroupTypeToString(sourceGroupSettings->getType()) +
                  "\": no factory module supports it.");
    }
  }
  return sourceGroups;
}

bool SourceGroupFactory::supports(SourceGroupType type) const {
  for(const std::shared_ptr<SourceGroupFactoryModule>& module : m_modules) {
    if(module->supports(type)) {
      return true;
    }
  }
  return false;
}

std::shared_ptr<SourceGroup> SourceGroupFactory::createSourceGroup(std::shared_ptr<SourceGroupSettings> settings) {
  std::shared_ptr<SourceGroup> sourceGroup;

  for(const std::shared_ptr<SourceGroupFactoryModule>& module : m_modules) {
    if(module->supports(settings->getType())) {
      sourceGroup = module->createSourceGroup(settings);
      break;
    }
  }

  return sourceGroup;
}

std::shared_ptr<SourceGroupFactory> SourceGroupFactory::s_instance;
std::once_flag SourceGroupFactory::s_once;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

SourceGroupFactory::SourceGroupFactory() {}
