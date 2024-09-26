#include "SnippetMerger.h"

#include <algorithm>

#include <range/v3/algorithm/sort.hpp>

#include "IApplicationSettings.hpp"

SnippetMerger::SnippetMerger(int startRow, int endRow) : m_start(startRow), m_end(endRow) {}

void SnippetMerger::addChild(std::shared_ptr<SnippetMerger> child) {
  m_children.push_back(std::move(child));
}

std::deque<SnippetMerger::Range> SnippetMerger::merge(const std::vector<SnippetMerger::Range>& atomicRanges) const {
  const int snippetExpandRange = IApplicationSettings::getInstanceRaw()->getCodeSnippetExpandRange();
  std::deque<Range> merged;
  if(m_children.empty()) {
    merged.emplace_back(Border(m_start, false), Border(m_end, false));
  } else {
    for(const auto& child : m_children) {
      std::deque<Range> mergedFromChild = child->merge(atomicRanges);
      std::transform(std::cbegin(mergedFromChild),
                     std::cend(mergedFromChild),
                     std::back_inserter(merged),
                     [this, snippetExpandRange, atomicRanges](const auto& mergeChild) {
                       return getExpandedRegardingAtomicRanges(mergeChild, snippetExpandRange, atomicRanges);
                     });
    }
    ranges::sort(merged, [](const Range& item0, const Range& item1) { return item0.start.row < item1.start.row; });

    // merge children
    const int snippetMergeRange = 2 * snippetExpandRange + 1;    // +1 since snippets that end/start with consecutive
                                                                 // lines should be merged as well.
    merged = Range::mergeAdjacent(merged, snippetMergeRange);

    // snap to own borders
    const int snippetSnapRange = IApplicationSettings::getInstanceRaw()->getCodeSnippetSnapRange();
    if((m_start < merged.front().start.row) && (merged.front().start.row <= m_start + snippetSnapRange)) {
      merged.front().start.row = m_start;
      merged.front().start.strong = false;
    }
    if((m_end - snippetSnapRange <= merged.back().end.row) && (merged.back().end.row < m_end)) {
      merged.back().end.row = m_end;
      merged.back().end.strong = false;
    }
  }
  return merged;
}

SnippetMerger::Range SnippetMerger::getExpandedRegardingAtomicRanges(Range range,
                                                                     const int snippetExpandRange,
                                                                     const std::vector<Range>& atomicRanges) const {
  const int rangeStartThreshold = range.start.row - snippetExpandRange;
  const int rangeEndThreshold = range.end.row + snippetExpandRange;
  for(auto atomicRange : atomicRanges) {
    if((!range.start.strong) && (atomicRange.end.row >= rangeStartThreshold) && (atomicRange.start.row < rangeStartThreshold)) {
      range.start.row = std::min(range.start.row, atomicRange.start.row);
      range.start.strong = true;
    }
    if((!range.end.strong) && (atomicRange.start.row <= rangeEndThreshold) && (atomicRange.end.row > rangeEndThreshold)) {
      range.end.row = std::max(range.end.row, atomicRange.end.row);
      range.end.strong = true;
    }
  }
  return range;
}
