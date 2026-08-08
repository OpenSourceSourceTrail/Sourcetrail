#include "Capabilities.h"

#include <algorithm>

#include "EngineCall.h"

namespace client {

Capabilities& Capabilities::instance() {
  static Capabilities sInstance;
  return sInstance;
}

void Capabilities::setChannel(EngineChannel* channel) {
  mChannel = channel;
  mCached.reset();
}

void Capabilities::invalidate() {
  mCached.reset();
}

const sourcetrail::CapabilitiesResponse* Capabilities::get() const {
  if(!mCached) {
    // A failure leaves the cache empty on purpose: the next query retries, so capabilities come back
    // by themselves once the supervisor has the engine running again.
    mCached = call(mChannel, "GetCapabilities", &Stub::GetCapabilities, sourcetrail::EmptyRequest{});
  }
  return mCached ? &*mCached : nullptr;
}

bool Capabilities::canCreateProject() const {
  const sourcetrail::CapabilitiesResponse* capabilities = get();
  return capabilities != nullptr && capabilities->can_create_project();
}

bool Capabilities::supportsSourceGroupType(SourceGroupType type) const {
  const sourcetrail::CapabilitiesResponse* capabilities = get();
  if(capabilities == nullptr) {
    return false;
  }
  const auto& types = capabilities->supported_source_group_types();
  return std::ranges::find(types, static_cast<int>(type)) != types.end();
}

}    // namespace client
