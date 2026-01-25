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

#define TKIT_DEBUG_BREAK_IF(condition)                                                                               \
    if (condition)                                                                                                   \
    TKIT_DEBUG_BREAK()

#define TKIT_LOG_COLOR_FATAL "\033[1;31m"

#ifdef TKIT_ENABLE_ASSERTS
namespace TKit::Detail
{
template <typename... Args>
void LogAndBreak(const char *level, const char *color, const char *file, const i32 line,
                 const fmt::format_string<Args...> message, Args &&...args)
{
    Log(message, level, color, file, line, std::forward<Args>(args)...);
    TKIT_DEBUG_BREAK();
}
void LogAndBreak(const char *level, const char *color, const char *file, i32 line);
void CheckOutOfBounds(const char *level, const char *color, const char *file, i32 line, usize index,
                      usize size, std::string_view head = "");
} // namespace TKit::Detail
#endif

#ifdef TKIT_ENABLE_ASSERTS
#    define TKIT_FATAL(...)                                                                                            \
        TKit::Detail::LogAndBreak("FATAL", TKIT_LOG_COLOR_FATAL, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_ASSERT(condition, ...)                                                                              \
        if (!(condition))                                                                                            \
        TKit::Detail::LogAndBreak("FATAL", TKIT_LOG_COLOR_FATAL, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_CHECK_OUT_OF_BOUNDS(index, size, ...)                                                             \
        if ((index) >= (size))                                                                                     \
            TKit::Detail::CheckOutOfBounds("FATAL", TKIT_LOG_COLOR_FATAL, __FILE__, __LINE__, index,                 \
                                           size __VA_OPT__(, ) __VA_ARGS__);

#    define TKIT_CHECK_RETURNS(expression, expected, ...)                                                          \
        TKIT_ASSERT((expression) == (expected)__VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_CHECK_NOT_RETURNS(expression, expected, ...)                                                      \
        TKIT_ASSERT((expression) != (expected)__VA_OPT__(, ) __VA_ARGS__)
#else
#    define TKIT_FATAL(message, ...)
#    define TKIT_ASSERT(condition, message, ...)

#    define TKIT_CHECK_RETURNS(expression, expected, ...) expression
#    define TKIT_CHECK_NOT_RETURNS(expression, expected, ...) expression
#    define TKIT_CHECK_OUT_OF_BOUNDS(index, size, ...)
#endif
