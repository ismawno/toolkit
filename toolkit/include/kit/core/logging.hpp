#pragma once

#include "kit/core/core.hpp"
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
KIT_API void debugBreak() KIT_NOEXCEPT;
KIT_API void logMessage(const char *p_Level, std::string_view p_File, const i32 p_Line, const char *p_Color,
                        const bool p_Crash, std::string_view p_Message) KIT_NOEXCEPT;
KIT_API void logMessageIf(bool condition, const char *p_Level, std::string_view p_File, const i32 p_Line,
                          const char *p_Color, const bool p_Crash, std::string_view p_Message) KIT_NOEXCEPT;
#endif
} // namespace KIT

#ifdef KIT_ENABLE_EXCEPTIONS
#    define KIT_CRASH(msg) throw std::runtime_error(msg)
#else
#    define KIT_CRASH(msg) KIT::debugBreak()
#endif

#ifdef KIT_ENABLE_INFO_LOGS
#    define KIT_LOG_INFO(...)                                                                                          \
        KIT::logMessage("INFO", __FILE__, __LINE__, KIT_LOG_COLOR_GREEN, false, KIT_FORMAT(__VA_ARGS__))
#    define KIT_LOG_INFO_IF(condition, ...)                                                                            \
        KIT::logMessageIf(condition, "INFO", __FILE__, __LINE__, KIT_LOG_COLOR_GREEN, false, KIT_FORMAT(__VA_ARGS__))
#else
#    define KIT_LOG_INFO(...)
#    define KIT_LOG_INFO_IF(...)
#endif

#ifdef KIT_ENABLE_WARNING_LOGS
#    define KIT_LOG_WARNING(...)                                                                                       \
        KIT::logMessage("WARNING", __FILE__, __LINE__, KIT_LOG_COLOR_YELLOW, false, KIT_FORMAT(__VA_ARGS__))
#    define KIT_LOG_WARNING_IF(condition, ...)                                                                         \
        KIT::logMessageIf(condition, "WARNING", __FILE__, __LINE__, KIT_LOG_COLOR_YELLOW, false,                       \
                          KIT_FORMAT(__VA_ARGS__))
#else
#    define KIT_LOG_WARNING(...)
#    define KIT_LOG_WARNING_IF(...)
#endif

#ifdef KIT_ENABLE_ASSERTS
#    ifndef KIT_SILENT_ASSERTS
#        define KIT_ERROR(...)                                                                                         \
            KIT::logMessage("ERROR", __FILE__, __LINE__, KIT_LOG_COLOR_RED, true, KIT_FORMAT(__VA_ARGS__))
#        define KIT_ASSERT(condition, ...)                                                                             \
            KIT::logMessageIf(!(condition), "ERROR", __FILE__, __LINE__, KIT_LOG_COLOR_RED, true,                      \
                              KIT_FORMAT(__VA_ARGS__))
#    else
#        define KIT_ERROR(...) KIT_CRASH(KIT_FORMAT(__VA_ARGS__))
#        define KIT_ASSERT(condition, ...)                                                                             \
            if (!(condition))                                                                                          \
            KIT_CRASH(KIT_FORMAT(__VA_ARGS__))
#    endif
#    define KIT_ASSERT_RETURNS(expression, expected, ...) KIT_ASSERT((expression) == (expected), __VA_ARGS__)
#else
#    define KIT_ERROR(...)
#    define KIT_ASSERT(...)
#    define KIT_ASSERT_RETURNS(expression, expected, ...) expression
#endif