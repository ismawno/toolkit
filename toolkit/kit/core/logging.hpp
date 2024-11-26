#pragma once

#include "kit/core/api.hpp"
#include "kit/core/alias.hpp"

#if !defined(TKIT_ENABLE_INFO_LOGS) && !defined(TKIT_ENABLE_WARNING_LOGS) && !defined(TKIT_ENABLE_ASSERTS)
#    define TKIT_NO_LOGS
#endif

// We dont use enough formatting features to justify the overhead of fmtlib, but linux doesnt have std::format, so I
// am forced to use it in that case
#ifndef TKIT_NO_LOGS
#    ifdef TKIT_OS_LINUX
#        include <fmt/format.h>
#        define TKIT_FORMAT(...) fmt::format(__VA_ARGS__)
#    else
#        include <format>
#        define TKIT_FORMAT(...) std::format(__VA_ARGS__)
#    endif
#endif

// sonarlint yells at me bc of this but idgaf
#ifdef TKIT_ENABLE_LOG_COLORS
#    define TKIT_LOG_COLOR_RESET "\033[0m"
#    define TKIT_LOG_COLOR_RED "\033[31m"
#    define TKIT_LOG_COLOR_GREEN "\033[32m"
#    define TKIT_LOG_COLOR_YELLOW "\033[33m"
#    define TKIT_LOG_COLOR_BLUE "\033[34m"
#else
#    define TKIT_LOG_COLOR_RESET ""
#    define TKIT_LOG_COLOR_RED ""
#    define TKIT_LOG_COLOR_GREEN ""
#    define TKIT_LOG_COLOR_YELLOW ""
#    define TKIT_LOG_COLOR_BLUE ""
#endif

namespace TKit
{
#ifndef TKIT_NO_LOGS
// These are not meant to be used directly, use the macros below instead
TKIT_API void debugBreak() noexcept;
TKIT_API void logMessage(const char *p_Level, std::string_view p_File, const i32 p_Line, const char *p_Color,
                         const bool p_Crash, std::string_view p_Message) noexcept;
TKIT_API void logMessageIf(bool p_Condition, const char *p_Level, std::string_view p_File, const i32 p_Line,
                           const char *p_Color, const bool p_Crash, std::string_view p_Message) noexcept;
#endif
} // namespace TKit

#ifdef TKIT_ENABLE_INFO_LOGS
#    define TKIT_LOG_INFO(...)                                                                                         \
        TKit::logMessage("INFO", __FILE__, INT32_MAX, TKIT_LOG_COLOR_GREEN, false, TKIT_FORMAT(__VA_ARGS__))
#    define TKIT_LOG_INFO_IF(p_Condition, ...)                                                                         \
        TKit::logMessageIf(p_Condition, "INFO", __FILE__, INT32_MAX, TKIT_LOG_COLOR_GREEN, false,                      \
                           TKIT_FORMAT(__VA_ARGS__))
#else
#    define TKIT_LOG_INFO(...)
#    define TKIT_LOG_INFO_IF(...)
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define TKIT_LOG_WARNING(...)                                                                                      \
        TKit::logMessage("WARNING", __FILE__, __LINE__, TKIT_LOG_COLOR_YELLOW, false, TKIT_FORMAT(__VA_ARGS__))
#    define TKIT_LOG_WARNING_IF(p_Condition, ...)                                                                      \
        TKit::logMessageIf(p_Condition, "WARNING", __FILE__, __LINE__, TKIT_LOG_COLOR_YELLOW, false,                   \
                           TKIT_FORMAT(__VA_ARGS__))
#else
#    define TKIT_LOG_WARNING(...)
#    define TKIT_LOG_WARNING_IF(...)
#endif

#ifdef TKIT_ENABLE_ASSERTS

#    ifndef TKIT_WEAK_ASSERTS
#        define TKIT_DEBUG_BREAK_IF(p_Condition)                                                                       \
            if (p_Condition)                                                                                           \
            TKit::debugBreak()

#        define TKIT_DEBUG_BREAK() TKit::debugBreak()
#    else
#        define TKIT_DEBUG_BREAK_IF(p_Condition)
#        define TKIT_DEBUG_BREAK()
#    endif

#    ifndef TKIT_SILENT_ASSERTS
#        define TKIT_ERROR(...)                                                                                        \
            TKit::logMessage("ERROR", __FILE__, __LINE__, TKIT_LOG_COLOR_RED, true, TKIT_FORMAT(__VA_ARGS__))
#        define TKIT_ASSERT(p_Condition, ...)                                                                          \
            TKit::logMessageIf(!(p_Condition), "ERROR", __FILE__, __LINE__, TKIT_LOG_COLOR_RED, true,                  \
                               TKIT_FORMAT(__VA_ARGS__))
#    else
#        define TKIT_ERROR(...) TKIT_DEBUG_BREAK()
#        define TKIT_ASSERT(p_Condition, ...) TKIT_DEBUG_BREAK_IF(!(p_Condition))
#    endif
#    define TKIT_ASSERT_RETURNS(expression, expected, ...) TKIT_ASSERT((expression) == (expected), __VA_ARGS__)
#else
#    define TKIT_ERROR(...)
#    define TKIT_ASSERT(...)
#    define TKIT_ASSERT_RETURNS(expression, expected, ...) expression
#endif
