#pragma once
#include <fmt/xchar.h>

#include <spdlog/spdlog.h>

#include "utilityString.h"

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
#  warning "\"SOURCE_PATH_PREFIX_LEN\" is missing, Full path will be used"
#  define REL_FILE_PATH __FILE__
#endif
/**
 * @brief Macros to simplify usage of the log mManager
 */
#define LOG_TRACE(__format__, ...)                                                                                               \
  spdlog::trace(std::string{"{} {}:{} "} + __format__, REL_FILE_PATH, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef _WIN32
#  define LOG_TRACE_W(__format__, ...)                                                                                           \
    spdlog::trace(std::wstring{L"{} {}:{} "} + __format__,                                                                       \
                  utility::decodeFromUtf8(REL_FILE_PATH),                                                                        \
                  utility::decodeFromUtf8(__FUNCTION__),                                                                         \
                  __LINE__,                                                                                                      \
                  ##__VA_ARGS__)
#else
#  define LOG_TRACE_W(__format__, ...) LOG_TRACE(utility::encodeToUtf8(__format__), ##__VA_ARGS__)
#endif

#define LOG_DEBUG(__format__, ...)                                                                                               \
  spdlog::debug(std::string{"{} {}:{} "} + __format__, REL_FILE_PATH, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef _WIN32
#  define LOG_DEBUG_W(__format__, ...)                                                                                           \
    spdlog::debug(std::wstring{L"{} {}:{} "} + __format__,                                                                       \
                  utility::decodeFromUtf8(REL_FILE_PATH),                                                                        \
                  utility::decodeFromUtf8(__FUNCTION__),                                                                         \
                  __LINE__,                                                                                                      \
                  ##__VA_ARGS__)
#else
#  define LOG_DEBUG_W(__format__, ...) LOG_DEBUG(utility::encodeToUtf8(__format__), ##__VA_ARGS__)
#endif


#define LOG_INFO(__format__, ...)                                                                                                \
  spdlog::info(std::string{"{} {}:{} "} + __format__, REL_FILE_PATH, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef _WIN32
#  define LOG_INFO_W(__format__, ...)                                                                                            \
    spdlog::info(std::wstring{L"{} {}:{} "} + __format__,                                                                        \
                 utility::decodeFromUtf8(REL_FILE_PATH),                                                                         \
                 utility::decodeFromUtf8(__FUNCTION__),                                                                          \
                 __LINE__,                                                                                                       \
                 ##__VA_ARGS__)
#else
#  define LOG_INFO_W(__format__, ...) LOG_INFO(utility::encodeToUtf8(__format__), ##__VA_ARGS__)
#endif

#define LOG_WARNING(__format__, ...)                                                                                             \
  spdlog::warn(std::string{"{} {}:{} "} + __format__, REL_FILE_PATH, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef _WIN32
#  define LOG_WARNING_W(__format__, ...)                                                                                         \
    spdlog::warn(std::wstring{L"{} {}:{} "} + __format__,                                                                        \
                 utility::decodeFromUtf8(REL_FILE_PATH),                                                                         \
                 utility::decodeFromUtf8(__FUNCTION__),                                                                          \
                 __LINE__,                                                                                                       \
                 ##__VA_ARGS__);
#else
#  define LOG_WARNING_W(__format__, ...) LOG_WARNING(utility::encodeToUtf8(__format__), ##__VA_ARGS__)
#endif

#define LOG_ERROR(__format__, ...)                                                                                               \
  spdlog::error(std::string{"{} {}:{} "} + __format__, REL_FILE_PATH, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef _WIN32
#  define LOG_ERROR_W(__format__, ...)                                                                                           \
    spdlog::error(std::wstring{L"{} {}:{} "} + __format__,                                                                       \
                  utility::decodeFromUtf8(REL_FILE_PATH),                                                                        \
                  utility::decodeFromUtf8(__FUNCTION__),                                                                         \
                  __LINE__,                                                                                                      \
                  ##__VA_ARGS__)
#else
#  define LOG_ERROR_W(__format__, ...) LOG_ERROR(utility::encodeToUtf8(__format__), ##__VA_ARGS__)
#endif

#define LOG_CRITICAL(__format__, ...)                                                                                            \
  spdlog::critical(std::string{"{} {}:{} "} + __format__, REL_FILE_PATH, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef _WIN32
#  define LOG_CRITICAL_W(__format__, ...)                                                                                        \
    spdlog::critical(std::wstring{L"{} {}:{} "} + __format__,                                                                    \
                     utility::decodeFromUtf8(REL_FILE_PATH),                                                                     \
                     utility::decodeFromUtf8(__FUNCTION__),                                                                      \
                     __LINE__,                                                                                                   \
                     ##__VA_ARGS__)
#else
#  define LOG_CRITICAL_W(__format__, ...) LOG_CRITICAL(utility::encodeToUtf8(__format__), ##__VA_ARGS__)
#endif
