#pragma once
#include <fmt/xchar.h>

#include <spdlog/spdlog.h>

#include "utilityString.h"

namespace logging::internal {

/**
 * @brief Logs a formatted message with specified log level and source location information
 *
 * @tparam FORMAT String type for the format message (std::string, std::string_view, const char*,
 *                std::wstring, std::wstring_view, or const wchar_t*)
 * @tparam Args Variadic template parameter pack for format arguments
 *
 * @param level The logging level (from spdlog::level::level_enum)
 * @param fileName Source file name where the log was called
 * @param function Function name where the log was called
 * @param line Line number where the log was called
 * @param format Format string for the log message
 * @param args Variable number of arguments to be formatted into the message
 *
 * @note For wide string formats (std::wstring, std::wstring_view, const wchar_t*),
 *       behavior depends on SPDLOG_WCHAR_TO_UTF8_SUPPORT definition:
 *       - If defined: Converts file and function names to wide strings
 *       - If not defined: Converts the formatted message to UTF-8
 *
 * @requires FORMAT must be one of: std::string, std::string_view, const char*,
 *           std::wstring, std::wstring_view, or const wchar_t*
 */
template <typename FORMAT, typename... Args>
  requires(std::same_as<FORMAT, std::string> || std::same_as<FORMAT, std::string_view> || std::same_as<FORMAT, const char*> ||
           std::same_as<FORMAT, std::wstring> || std::same_as<FORMAT, std::wstring_view> || std::same_as<FORMAT, const wchar_t*>)
void log(spdlog::level::level_enum level,
         const char* fileName,
         const char* function,
         std::uintmax_t line,
         FORMAT format,
         const Args&... args) {
  using namespace std::string_literals;
  if constexpr(std::same_as<FORMAT, std::string> || std::same_as<FORMAT, std::string_view> || std::same_as<FORMAT, const char*>) {
    spdlog::log(level, fmt::runtime("{} {}:{} "s + format), fileName, function, line, args...);
  } else if constexpr(std::same_as<FORMAT, std::wstring> || std::same_as<FORMAT, std::wstring_view> ||
                      std::same_as<FORMAT, const wchar_t*>) {
#ifdef SPDLOG_WCHAR_TO_UTF8_SUPPORT
    spdlog::log(level,
                fmt::runtime(L"{} {}:{} "s + format),
                utility::decodeFromUtf8(fileName),
                utility::decodeFromUtf8(function),
                line,
                args...);
#else
    spdlog::log(level, "{} {}:{} {}", fileName, function, line, utility::encodeToUtf8(fmt::format(fmt::runtime(format), args...)));
#endif
  }
}

}    // namespace logging::internal


/**
 * Trace: For extremely detailed debugging information.
 * Debug: For diagnostic information useful during development.
 * Info: For general application flow and milestones.
 * Warn: For unexpected but recoverable situations.
 * Error: For errors that impact functionality but allow the application to continue.
 * Fatal/Critical: For severe errors that cause the application to terminate.
 */
#ifdef SOURCE_PATH_PREFIX_LEN
#  define REL_FILE_PATH std::next(__FILE__, static_cast<long>(SOURCE_PATH_PREFIX_LEN))
#else
#  pragma message "\"SOURCE_PATH_PREFIX_LEN\" is missing, Full path will be used"
#  define REL_FILE_PATH __FILE__
#endif

// clang-format off
/**
 * @brief Macros to simplify usage of the log Manager
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TRACE(Format, ...) logging::internal::log(spdlog::level::trace, REL_FILE_PATH, __FUNCTION__, __LINE__, Format __VA_OPT__(,) __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TRACE_W(Format, ...) LOG_TRACE(Format __VA_OPT__(,) __VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_DEBUG(Format, ...) logging::internal::log(spdlog::level::debug, REL_FILE_PATH, __FUNCTION__, __LINE__, Format __VA_OPT__(,) __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_DEBUG_W(Format, ...) LOG_DEBUG(Format __VA_OPT__(,) __VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INFO(Format, ...) logging::internal::log(spdlog::level::info, REL_FILE_PATH, __FUNCTION__, __LINE__, Format __VA_OPT__(,) __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INFO_W(Format, ...) LOG_INFO(Format __VA_OPT__(,) __VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_WARNING(Format, ...) logging::internal::log(spdlog::level::warn, REL_FILE_PATH, __FUNCTION__, __LINE__, Format __VA_OPT__(,) __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_WARNING_W(Format, ...) LOG_WARNING(Format __VA_OPT__(,) __VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ERROR(Format, ...) logging::internal::log(spdlog::level::err, REL_FILE_PATH, __FUNCTION__, __LINE__, Format __VA_OPT__(,) __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ERROR_W(Format, ...) LOG_ERROR(Format __VA_OPT__(,) __VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_CRITICAL(Format, ...) logging::internal::log(spdlog::level::critical, REL_FILE_PATH, __FUNCTION__, __LINE__, Format __VA_OPT__(,) __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_CRITICAL_W(Format, ...) LOG_CRITICAL(Format __VA_OPT__(,) __VA_ARGS__)
// clang-format on
