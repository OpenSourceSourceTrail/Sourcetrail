#pragma once
#include <optional>

#include "engine.pb.h"
#include "settings/source_group/SourceGroupType.h"

class EngineChannel;

namespace client {

/**
 * What the connected engine can index, cached client-side.
 *
 * A client must not look at the plugin directory itself -- the engine may run elsewhere, and the
 * plugin registry lives on its side of the boundary. Everything the GUI needs to enable or grey out
 * a control comes from one GetCapabilities call, fetched on first use and kept until invalidate().
 *
 * With no engine reachable there are no capabilities, which is exactly read-only mode: opening and
 * navigating an already-indexed project needs no indexer, so only the creating and refreshing
 * controls go dead.
 */
class Capabilities final {
public:
  /** Global because the project wizard's leaf widgets ask, and threading a pointer to each is noise. */
  static Capabilities& instance();

  void setChannel(EngineChannel* channel);

  /** Drops the cache so the next query re-asks; call after the engine restarts. */
  void invalidate();

  /** The engine connection the client side shares; null while no engine is reachable. */
  [[nodiscard]] EngineChannel* channel() const {
    return mChannel;
  }

  [[nodiscard]] bool canCreateProject() const;
  [[nodiscard]] bool supportsSourceGroupType(SourceGroupType type) const;

private:
  Capabilities() = default;

  /** Null while the engine is unreachable. */
  const sourcetrail::CapabilitiesResponse* get() const;

  EngineChannel* mChannel = nullptr;
  mutable std::optional<sourcetrail::CapabilitiesResponse> mCached;
};

}    // namespace client
