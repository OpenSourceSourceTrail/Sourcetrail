#include "project/logic/SourceGroupFactoryModuleJava.h"

#include "project/logic/SourceGroupJavaEmpty.h"
#include "settings/source_group/type/SourceGroupSettingsJavaEmpty.h"

bool SourceGroupFactoryModuleJava::supports(SourceGroupType type) const {
  switch(type) {
  case SOURCE_GROUP_JAVA_EMPTY:
    return true;
  default:
    break;
  }
  return false;
}

std::shared_ptr<SourceGroup> SourceGroupFactoryModuleJava::createSourceGroup(std::shared_ptr<SourceGroupSettings> settings) const {
  std::shared_ptr<SourceGroup> sourceGroup;
  if(std::shared_ptr<SourceGroupSettingsJavaEmpty> javaSettings = std::dynamic_pointer_cast<SourceGroupSettingsJavaEmpty>(settings)) {
    sourceGroup = std::shared_ptr<SourceGroup>(new SourceGroupJavaEmpty(javaSettings));
  }
  return sourceGroup;
}
