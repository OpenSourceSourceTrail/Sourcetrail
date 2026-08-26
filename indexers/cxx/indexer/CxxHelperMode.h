#pragma once
#include <string>

namespace helper {

/**
 * Answers one CxxToolchainRequest read from `requestFilePath`, writing the response to
 * `responseFilePath`. This is how a process without Clang -- the engine -- gets a compilation
 * database parsed or a precompiled header built. Returns an exit code.
 */
int run(const std::string& requestFilePath, const std::string& responseFilePath);

}    // namespace helper
