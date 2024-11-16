#pragma once

#include "kit/core/api.hpp"
#include "kit/core/alias.hpp"

#if !defined(KIT_ENABLE_INFO_LOGS) && !defined(KIT_ENABLE_WARNING_LOGS) && !defined(KIT_ENABLE_ASSERTS)
#    define KIT_NO_LOGS
#endif

// We dont use enough formatting features to justify the overhead of fmtlib, but linux doesnt have std::format, so I
// am forced to use it in that case
#ifndef KIT_NO_LOGS
#    ifdef KIT_OS_LINUX
#        include <fmt/format.h>
#        define KIT_FORMAT(...) fmt::format(__VA_ARGS__)
#    else
#        include <format>
#        define KIT_FORMAT(...) std::format(__VA_ARGS__)
#    endif
#endif

#ifndef KIT_ROOT_LOG_PATH
#    define KIT_ROOT_LOG_PATH KIT_ROOT_PATH
#endif

// sonarlint yells at me bc of this but idgaf
#ifdef KIT_ENABLE_LOG_COLORS
#    define KIT_LOG_COLOR_RESET "\033[0m"
#    define KIT_LOG_COLOR_RED "\033[31m"
#    define KIT_LOG_COLOR_GREEN "\033[32m"
#    define KIT_LOG_COLOR_YELLOW "\033[33m"
#    define KIT_LOG_COLOR_BLUE "\033[34m"
#else
#    define KIT_LOG_COLOR_RESET ""
#    define KIT_LOG_COLOR_RED ""
#    define KIT_LOG_COLOR_GREEN ""
#    define KIT_LOG_COLOR_YELLOW ""
#    define KIT_LOG_COLOR_BLUE ""
#endif

namespace KIT
{
#ifndef KIT_NO_LOGS
// These are not meant to be used directly, use the macros below instead
KIT_API void debugBreak() noexcept;
KIT_API void logMessage(const char *p_Level, std::string_view p_File, const i32 p_Line, const char *p_Color,
                        const bool p_Crash, std::string_view p_Message) noexcept;
KIT_API void logMessageIf(bool p_Condition, const char *p_Level, std::string_view p_File, const i32 p_Line,
                          const char *p_Color, const bool p_Crash, std::string_view p_Message) noexcept;
#endif
} // namespace KIT

#ifdef KIT_ENABLE_INFO_LOGS
#    define KIT_LOG_INFO(...)                                                                                          \
        KIT::logMessage("INFO", __FILE__, INT32_MAX, KIT_LOG_COLOR_GREEN, false, KIT_FORMAT(__VA_ARGS__))
#    define KIT_LOG_INFO_IF(p_Condition, ...)                                                                          \
        KIT::logMessageIf(p_Condition, "INFO", __FILE__, INT32_MAX, KIT_LOG_COLOR_GREEN, false, KIT_FORMAT(__VA_ARGS__))
#else
#    define KIT_LOG_INFO(...)
#    define KIT_LOG_INFO_IF(...)
#endif

#ifdef KIT_ENABLE_WARNING_LOGS
#    define KIT_LOG_WARNING(...)                                                                                       \
        KIT::logMessage("WARNING", __FILE__, __LINE__, KIT_LOG_COLOR_YELLOW, false, KIT_FORMAT(__VA_ARGS__))
#    define KIT_LOG_WARNING_IF(p_Condition, ...)                                                                       \
        KIT::logMessageIf(p_Condition, "WARNING", __FILE__, __LINE__, KIT_LOG_COLOR_YELLOW, false,                     \
                          KIT_FORMAT(__VA_ARGS__))
#else
#    define KIT_LOG_WARNING(...)
#    define KIT_LOG_WARNING_IF(...)
#endif

#ifdef KIT_ENABLE_ASSERTS

#    ifndef KIT_WEAK_ASSERTS
#        define KIT_DEBUG_BREAK_IF(p_Condition)                                                                        \
            if (p_Condition)                                                                                           \
            KIT::debugBreak()

#        define KIT_DEBUG_BREAK() KIT::debugBreak()
#    else
#        define KIT_DEBUG_BREAK_IF(p_Condition)
#        define KIT_DEBUG_BREAK()
#    endif

#    ifndef KIT_SILENT_ASSERTS
#        define KIT_ERROR(...)                                                                                         \
            KIT::logMessage("ERROR", __FILE__, __LINE__, KIT_LOG_COLOR_RED, true, KIT_FORMAT(__VA_ARGS__))
#        define KIT_ASSERT(p_Condition, ...)                                                                           \
            KIT::logMessageIf(!(p_Condition), "ERROR", __FILE__, __LINE__, KIT_LOG_COLOR_RED, true,                    \
                              KIT_FORMAT(__VA_ARGS__))
#    else
#        define KIT_ERROR(...) KIT_DEBUG_BREAK()
#        define KIT_ASSERT(p_Condition, ...) KIT_DEBUG_BREAK_IF(!(p_Condition))
#    endif
#    define KIT_ASSERT_RETURNS(expression, expected, ...) KIT_ASSERT((expression) == (expected), __VA_ARGS__)
#else
#    define KIT_ERROR(...)
#    define KIT_ASSERT(...)
#    define KIT_ASSERT_RETURNS(expression, expected, ...) expression
#endif
