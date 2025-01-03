#pragma once
#include <fmt/xchar.h>

#include <spdlog/spdlog.h>

#include "utilityString.h"

/**
 * @brief Macros to simplify usage of the log mManager
 */
#define LOG_INFO(__str__)                                                                                                        \
  spdlog::info("{} {}:{} {}", std::next(__FILE__, static_cast<long>(SOURCE_PATH_PREFIX_LEN)), __FUNCTION__, __LINE__, __str__);

#ifdef D_WINDOWS
#  define LOG_INFO_W(__str__)                                                                                                    \
    spdlog::info(L"{} {}:{} {}",                                                                                                 \
                 utility::decodeFromUtf8(std::next(__FILE__, static_cast<long>(SOURCE_PATH_PREFIX_LEN))),                        \
                 utility::decodeFromUtf8(__FUNCTION__),                                                                          \
                 __LINE__,                                                                                                       \
                 __str__);
#else
#  define LOG_INFO_W(__str__) LOG_INFO(utility::encodeToUtf8(__str__))
#endif

#define LOG_WARNING(__str__)                                                                                                     \
  spdlog::warn("{} {}:{} {}", std::next(__FILE__, static_cast<long>(SOURCE_PATH_PREFIX_LEN)), __FUNCTION__, __LINE__, __str__);

#ifdef D_WINDOWS
#  define LOG_WARNING_W(__str__)                                                                                                 \
    spdlog::warn(L"{} {}:{} {}",                                                                                                 \
                 utility::decodeFromUtf8(std::next(__FILE__, static_cast<long>(SOURCE_PATH_PREFIX_LEN))),                        \
                 utility::decodeFromUtf8(__FUNCTION__),                                                                          \
                 __LINE__,                                                                                                       \
                 __str__);
#else
#  define LOG_WARNING_W(__str__) LOG_WARNING(utility::encodeToUtf8(__str__));
#endif

#define LOG_ERROR(__str__)                                                                                                       \
  spdlog::error("{} {}:{} {}", std::next(__FILE__, static_cast<long>(SOURCE_PATH_PREFIX_LEN)), __FUNCTION__, __LINE__, __str__);

#ifdef D_WINDOWS
#  define LOG_ERROR_W(__str__)                                                                                                   \
    spdlog::error(L"{} {}:{} {}",                                                                                                \
                  utility::decodeFromUtf8(std::next(__FILE__, static_cast<long>(SOURCE_PATH_PREFIX_LEN))),                       \
                  utility::decodeFromUtf8(__FUNCTION__),                                                                         \
                  __LINE__,                                                                                                      \
                  __str__);
#else
#  define LOG_ERROR_W(__str__) LOG_ERROR(utility::encodeToUtf8(__str__));
#endif