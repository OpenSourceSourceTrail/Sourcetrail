#pragma once
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

class SnippetMerger final {
public:
  struct Border final {
    Border(int row_, bool strong_) : row(row_), strong(strong_) {}
    int row;
    bool strong;
  };

  struct Range final {
    template <template <class, class> class ContainerType>
    static ContainerType<Range, std::allocator<Range>> mergeAdjacent(ContainerType<Range, std::allocator<Range>> ranges,
                                                                     int rowDifference = 1) {
      for(size_t i = 0; i + 1 < ranges.size(); i++) {
        const Range first = ranges[i];
        const Range second = ranges[i + 1];
        if((first.end.row + rowDifference) >= (second.start.row)) {
          ranges.erase(ranges.begin() + static_cast<long>(i), ranges.begin() + static_cast<long>(i) + 2);
          ranges.insert(ranges.begin() + static_cast<long>(i),
                        Range(first.start.row < second.start.row ? first.start : second.start,
                              first.end.row > second.end.row ? first.end : second.end));
          i--;
        }
      }
      return ranges;
    }

    Range(Border start_, Border end_) : start(start_), end(end_) {}
    Border start;
    Border end;
  };

  SnippetMerger(int startRow, int endRow);
  void addChild(std::shared_ptr<SnippetMerger> child);
  [[nodiscard]] std::deque<Range> merge(const std::vector<SnippetMerger::Range>& atomicRanges) const;

private:
  [[nodiscard]] Range getExpandedRegardingAtomicRanges(Range range,
                                                       const int snippetExpandRange,
                                                       const std::vector<Range>& atomicRanges) const;

  const int m_start;
  const int m_end;
  std::vector<std::shared_ptr<SnippetMerger>> m_children;
};
