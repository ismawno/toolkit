#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"

#if !defined(KIT_ENABLE_INFO_LOGS) && !defined(KIT_ENABLE_WARNING_LOGS) && !defined(KIT_ENABLE_ASSERTS)
#    define KIT_NO_LOGS
#endif

// We dont use enough formatting features to justify the overhead of fmtlib, but linux doesnt have std::format, so we
// are forced to use it in that case
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

KIT_NAMESPACE_BEGIN

#ifndef KIT_NO_LOGS
// These are not meant to be used directly, use the macros below instead
void debugBreak() KIT_NOEXCEPT;
void logMessage(const char *p_Level, const String &p_File, const i32 p_Line, const char *p_Color, const bool p_Crash,
                const String &p_Message) KIT_NOEXCEPT;
void logMessageIf(bool condition, const char *p_Level, const String &p_File, const i32 p_Line, const char *p_Color,
                  const bool p_Crash, const String &p_Message) KIT_NOEXCEPT;
#endif

KIT_NAMESPACE_END

#ifdef KIT_THROW
#    define KIT_CRASH(msg) throw std::runtime_error(msg)
#else
#    define KIT_CRASH(msg) KIT_NAMESPACE_NAME::debugBreak()
#endif

#ifdef KIT_ENABLE_INFO_LOGS
#    define KIT_LOG_INFO(...)                                                                                          \
        KIT_NAMESPACE_NAME::logMessage("INFO", __FILE__, __LINE__, KIT_LOG_COLOR_GREEN, false, KIT_FORMAT(__VA_ARGS__))
#    define KIT_LOG_INFO_IF(condition, ...)                                                                            \
        KIT_NAMESPACE_NAME::logMessageIf(condition, "INFO", __FILE__, __LINE__, KIT_LOG_COLOR_GREEN, false,            \
                                         KIT_FORMAT(__VA_ARGS__))
#else
#    define KIT_LOG_INFO(...)
#    define KIT_LOG_INFO_IF(...)
#endif

#ifdef KIT_ENABLE_WARNING_LOGS
#    define KIT_LOG_WARNING(...)                                                                                       \
        KIT_NAMESPACE_NAME::logMessage("WARNING", __FILE__, __LINE__, KIT_LOG_COLOR_YELLOW, false,                     \
                                       KIT_FORMAT(__VA_ARGS__))
#    define KIT_LOG_WARNING_IF(condition, ...)                                                                         \
        KIT_NAMESPACE_NAME::logMessageIf(condition, "WARNING", __FILE__, __LINE__, KIT_LOG_COLOR_YELLOW, false,        \
                                         KIT_FORMAT(__VA_ARGS__))
#else
#    define KIT_LOG_WARNING(...)
#    define KIT_LOG_WARNING_IF(...)
#endif

#ifdef KIT_ENABLE_ASSERTS
#    ifndef KIT_SILENT_ASSERTS
#        define KIT_ERROR(...)                                                                                         \
            KIT_NAMESPACE_NAME::logMessage("ERROR", __FILE__, __LINE__, KIT_LOG_COLOR_RED, true,                       \
                                           KIT_FORMAT(__VA_ARGS__))
#        define KIT_ASSERT(condition, ...)                                                                             \
            KIT_NAMESPACE_NAME::logMessageIf(!(condition), "ERROR", __FILE__, __LINE__, KIT_LOG_COLOR_RED, true,       \
                                             KIT_FORMAT(__VA_ARGS__))
#    else
#        define KIT_ERROR(...) KIT_CRASH(KIT_FORMAT(__VA_ARGS__))
#        define KIT_ASSERT(condition, ...)                                                                             \
            if (!(condition))                                                                                          \
            KIT_CRASH(KIT_FORMAT(__VA_ARGS__))
#    endif
#else
#    define KIT_ERROR(...)
#    define KIT_ASSERT(...)
#endif