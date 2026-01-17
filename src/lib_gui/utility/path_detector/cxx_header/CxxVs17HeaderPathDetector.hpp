#pragma once
#include "PathDetector.h"


/**
 * Detects Visual Studio 2017+ header search paths using vswhere.exe.
 *
 * Note: Despite the class name, this detector works with VS 2017 and later,
 * which use the vswhere.exe tool. VS 2017 used a different detection mechanism.
 *
 * Detection process:
 * 1. Locate vswhere.exe in Program Files
 * 2. Query latest VS installation path
 * 3. Collect MSVC toolchain headers
 * 4. Add VS auxiliary headers
 * 5. Append Windows SDK headers based on system architecture
 */
class CxxVs17HeaderPathDetector final : public PathDetector {
public:
  CxxVs17HeaderPathDetector();

private:
  [[nodiscard]] std::vector<FilePath> doGetPaths() const override;
};
