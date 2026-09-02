#include "indexing/domain/IndexingPhaseStats.h"

#include <cstdio>

namespace indexing_stats {

void print() {
  std::fprintf(stderr,
               "INDEXER_TIMING pipeline merges=%zu merge_ms=%.1f merge_idle_ms=%.1f "
               "injects=%zu inject_ms=%.1f inject_idle_ms=%.1f max_queue_depth=%zu\n",
               merge.count.load(),
               merge.milliseconds.load(),
               mergeIdle.milliseconds.load(),
               inject.count.load(),
               inject.milliseconds.load(),
               injectIdle.milliseconds.load(),
               maxStorageQueueDepth.load());
  std::fprintf(stderr,
               "INDEXER_TIMING caches file_path_maps_ms=%.1f search_index_ms=%.1f "
               "member_edge_order_ms=%.1f hierarchy_ms=%.1f full_text_ms=%.1f optimize_ms=%.1f\n",
               filePathMaps.milliseconds.load(),
               searchIndex.milliseconds.load(),
               memberEdgeOrder.milliseconds.load(),
               hierarchyCache.milliseconds.load(),
               fullTextIndex.milliseconds.load(),
               optimizeDatabase.milliseconds.load());
  std::fflush(stderr);
}

}    // namespace indexing_stats
