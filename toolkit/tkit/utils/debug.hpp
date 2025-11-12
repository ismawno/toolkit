#pragma once

#include "tkit/utils/logging.hpp"

#ifdef TKIT_COMPILER_CLANG
#    define TKIT_DEBUG_BREAK() __builtin_debugtrap()
#elif defined(TKIT_COMPILER_GCC)
#    define TKIT_DEBUG_BREAK() __builtin_trap()
#elif defined(TKIT_COMPILER_MSVC)
#    define TKIT_DEBUG_BREAK() __debugbreak()
#elif defined(SIGTRAP)
#    define TKIT_DEBUG_BREAK() raise(SIGTRAP)
#elif defined(SIGABRT)
#    define TKIT_DEBUG_BREAK() raise(SIGABRT)
#endif

#define TKIT_DEBUG_BREAK_IF(p_Condition)                                                                               \
    if (p_Condition)                                                                                                   \
    TKIT_DEBUG_BREAK()

#define TKIT_LOG_COLOR_FATAL "\033[1;31m"

namespace TKit::Detail
{
#ifdef TKIT_ENABLE_ASSERTS
inline void LogAndBreak(const std::string_view p_Message, const char *p_Level, const char *p_Color, const char *p_File,
                        const i32 p_Line)
{
    Log(p_Message, p_Level, p_Color, p_File, p_Line);
    TKIT_DEBUG_BREAK();
}
#endif
} // namespace TKit::Detail

#ifdef TKIT_ENABLE_ASSERTS
#    define TKIT_FATAL(...)                                                                                            \
        TKit::Detail::LogAndBreak(TKit::Format(__VA_ARGS__), "FATAL", TKIT_LOG_COLOR_FATAL, __FILE__, __LINE__)
#    define TKIT_ASSERT(p_Condition, ...)                                                                              \
        if (!(p_Condition))                                                                                            \
        TKit::Detail::LogAndBreak(TKit::Format(__VA_ARGS__), "FATAL", TKIT_LOG_COLOR_FATAL, __FILE__, __LINE__)

#    define TKIT_ASSERT_RETURNS(expression, expected, ...) TKIT_ASSERT((expression) == (expected), __VA_ARGS__)
#    define TKIT_ASSERT_NOT_RETURNS(expression, expected, ...) TKIT_ASSERT((expression) != (expected), __VA_ARGS__)
#else
#    define TKIT_FATAL(p_Message, ...)
#    define TKIT_ASSERT(p_Condition, p_Message, ...)

#    define TKIT_ASSERT_RETURNS(expression, expected, ...) expression
#    define TKIT_ASSERT_NOT_RETURNS(expression, expected, ...) expression
#endif
