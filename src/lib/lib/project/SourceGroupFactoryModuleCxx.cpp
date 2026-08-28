#include "project/SourceGroupFactoryModuleCxx.h"

#include "project/SourceGroupCxxCdb.h"
#include "project/SourceGroupCxxEmpty.h"
#include "settings/source_group/type/SourceGroupSettingsCEmpty.h"
#include "settings/source_group/type/SourceGroupSettingsCppEmpty.h"
#include "settings/source_group/type/SourceGroupSettingsCxxCdb.h"

bool SourceGroupFactoryModuleCxx::supports(SourceGroupType type) const {
  switch(type) {
  case SOURCE_GROUP_C_EMPTY:
  case SOURCE_GROUP_CPP_EMPTY:
  case SOURCE_GROUP_CXX_CDB:
    return true;
  default:
    break;
  }
  return false;
}

std::shared_ptr<SourceGroup> SourceGroupFactoryModuleCxx::createSourceGroup(std::shared_ptr<SourceGroupSettings> settings) const {
  std::shared_ptr<SourceGroup> sourceGroup;
  if(std::shared_ptr<SourceGroupSettingsCxxCdb> cxxCdbSettings = std::dynamic_pointer_cast<SourceGroupSettingsCxxCdb>(settings)) {
    sourceGroup = std::shared_ptr<SourceGroup>(new SourceGroupCxxCdb(cxxCdbSettings));
  } else if(std::shared_ptr<SourceGroupSettingsCEmpty> cSettings = std::dynamic_pointer_cast<SourceGroupSettingsCEmpty>(settings)) {
    sourceGroup = std::shared_ptr<SourceGroup>(new SourceGroupCxxEmpty(cSettings));
  } else if(std::shared_ptr<SourceGroupSettingsCppEmpty> cxxSettings = std::dynamic_pointer_cast<SourceGroupSettingsCppEmpty>(
                settings)) {
    sourceGroup = std::shared_ptr<SourceGroup>(new SourceGroupCxxEmpty(cxxSettings));
  }
  return sourceGroup;
}
