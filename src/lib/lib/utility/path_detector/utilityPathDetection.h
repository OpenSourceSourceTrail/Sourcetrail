#pragma once

#include "CombinedPathDetector.h"

namespace utility {
std::shared_ptr<CombinedPathDetector> getCxxVsHeaderPathDetector();
std::shared_ptr<CombinedPathDetector> getCxxHeaderPathDetector();
std::shared_ptr<CombinedPathDetector> getCxxFrameworkPathDetector();
}    // namespace utility