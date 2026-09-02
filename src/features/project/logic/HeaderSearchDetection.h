#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "FilePath.h"
#include "FilePathFilter.h"
#include "utility/IncludeDirective.h"

class SourceGroupSettings;

namespace utility {

/**
 * Everything include detection needs, copied off the settings by the caller.
 *
 * Detection walks the source tree and parses every file, so it belongs on a worker thread -- and a
 * worker thread must not touch the settings object the wizard is still editing. Reading these
 * fields up front on the calling thread is what makes that safe.
 */
struct HeaderDetectionInputs {
  std::vector<FilePath> sourcePaths;
  std::vector<FilePathFilter> excludeFilters;
  std::vector<std::wstring> sourceExtensions;
  /** The global include paths plus this source group's own, already expanded and absolute. */
  std::vector<FilePath> headerSearchPaths;
};

/** Nullopt when the settings are not of a kind that carries source paths, so nothing can be detected. */
std::optional<HeaderDetectionInputs> collectDetectionInputs(const std::shared_ptr<SourceGroupSettings>& settings);

/**
 * Reports how far detection has got: a message, and a percentage when there is one.
 *
 * Nullopt means the step cannot say how long it will take. Whether that becomes a dialog, a status
 * bar or nothing at all is the caller's business -- this is the whole seam between detection and
 * whoever is watching it.
 */
using DetectionProgress = std::function<void(const std::wstring& message, std::optional<size_t> percent)>;

/** The #include directives in the source tree that none of the header search paths resolve. */
std::vector<IncludeDirective> findUnresolvedIncludes(const HeaderDetectionInputs& inputs, const DetectionProgress& progress);

/** The directories under `searchedPaths` that would resolve the source tree's #include directives. */
std::set<FilePath> detectHeaderSearchDirectories(const HeaderDetectionInputs& inputs,
                                                 const std::set<FilePath>& searchedPaths,
                                                 const DetectionProgress& progress);

/**
 * How many quantiles to split the source files into when sampling them.
 *
 * `log2` of the file count, which is 0 for an empty set -- `log2(0.0)` is -inf, and converting that
 * to an unsigned type is undefined behavior, so the empty case is answered before the maths.
 */
size_t detectionQuantileCount(size_t sourceFileCount);

}    // namespace utility
