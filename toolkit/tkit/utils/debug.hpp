#pragma once

#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/logging.hpp"

#ifdef TKIT_COMPILER_CLANG_GNU
#    define TKIT_DEBUG_BREAK() __builtin_debugtrap()
#    define TKIT_ASSUME(expr) __builtin_assume(expr)
#elif defined(TKIT_COMPILER_GCC)
#    define TKIT_DEBUG_BREAK() __builtin_trap()
#    define TKIT_ASSUME(expr)                                                                                          \
        if (!(expr))                                                                                                   \
        __builtin_unreachable()
#elif defined(TKIT_COMPILER_MSVC)
#    define TKIT_DEBUG_BREAK() __debugbreak()
#    define TKIT_ASSUME(expr) __assume(expr)
#elif defined(SIGTRAP)
#    define TKIT_DEBUG_BREAK() raise(SIGTRAP)
#    define TKIT_ASSUME(expr)
#elif defined(SIGABRT)
#    define TKIT_DEBUG_BREAK() raise(SIGABRT)
#    define TKIT_ASSUME(expr)
#else
#    error "[TOOLKIT][DEBUG] Unable to find a good definition for TKIT_DEBUG_BREAK()"
#endif

#define TKIT_DEBUG_BREAK_IF(condition)                                                                                 \
    if (condition)                                                                                                     \
    TKIT_DEBUG_BREAK()

#define TKIT_LOG_COLOR_PANIC "\033[1;31m"

#ifdef TKIT_ENABLE_ENSURE
namespace TKit::Detail
{
#    ifdef TKIT_ENABLE_STACK_TRACE
void PrintStackTrace(u32 skip);
#        define TKIT_PRINT_STACK_TRACE(skip) TKit::Detail::PrintStackTrace(skip)
#    else
#        define TKIT_PRINT_STACK_TRACE(skip)
#    endif

template <typename... Args>
void LogAndBreak(const char *level, const char *color, const char *file, const i32 line,
                 const fmt::format_string<Args...> message, Args &&...args)
{
    TKIT_PRINT_STACK_TRACE(1);
    Log(message, level, color, file, line, std::forward<Args>(args)...);
    TKIT_DEBUG_BREAK();
}
void LogAndBreak(const char *level, const char *color, const char *file, i32 line);
void CheckOutOfBounds(const char *level, const char *color, const char *file, i32 line, usize index, usize size,
                      std::string_view head = "");
} // namespace TKit::Detail

#    define TKIT_PANIC(...)                                                                                            \
        TKit::Detail::LogAndBreak("PANIC", TKIT_LOG_COLOR_PANIC, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_ENSURE(condition, ...)                                                                                \
        if (!(condition))                                                                                              \
        TKit::Detail::LogAndBreak("PANIC", TKIT_LOG_COLOR_PANIC, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_ENSURE_BOUNDS(index, size, ...)                                                                       \
        if ((index) >= (size))                                                                                         \
            TKit::Detail::CheckOutOfBounds("PANIC", TKIT_LOG_COLOR_PANIC, __FILE__, __LINE__, index,                   \
                                           size __VA_OPT__(, ) __VA_ARGS__);

#    define TKIT_ENSURE_RETURNS(expression, expected, ...)                                                             \
        TKIT_ENSURE((expression) == (expected)__VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_ENSURE_NOT_RETURNS(expression, expected, ...)                                                         \
        TKIT_ENSURE((expression) != (expected)__VA_OPT__(, ) __VA_ARGS__)
#else
#    define TKIT_PANIC(...)
#    define TKIT_ENSURE(...)
#    define TKIT_ENSURE_BOUNDS(...)

#    define TKIT_ENSURE_RETURNS(expression, expected, ...) expression
#    define TKIT_ENSURE_NOT_RETURNS(expression, expected, ...) expression
#endif

#ifdef TKIT_ENABLE_ASSERTS
#    ifdef TKIT_ENABLE_ASSERT_IS_ASSUME
#        define TKIT_FATAL(...) TKIT_UNREACHABLE()
#        define TKIT_ASSERT(condition, ...) TKIT_ASSUME(condition)
#        define TKIT_ASSERT_BOUNDS(index, size, ...) TKIT_ASSUME((index) < (size))

#        define TKIT_ASSERT_RETURNS(expression, expected, ...) TKIT_ASSUME((expression) == (expected))
#        define TKIT_ASSERT_NOT_RETURNS(expression, expected, ...) TKIT_ASSUME((expression) != (expected))
#    else
#        define TKIT_FATAL(...) TKIT_PANIC(__VA_ARGS__)
#        define TKIT_ASSERT(condition, ...) TKIT_ENSURE(condition, __VA_ARGS__)
#        define TKIT_ASSERT_BOUNDS(index, size, ...) TKIT_ENSURE_BOUNDS(index, size, __VA_ARGS__)

#        define TKIT_ASSERT_RETURNS(expression, expected, ...) TKIT_ENSURE_RETURNS(expression, expected, __VA_ARGS__)
#        define TKIT_ASSERT_NOT_RETURNS(expression, expected, ...)                                                     \
            TKIT_ENSURE_NOT_RETURNS(expression, expected, __VA_ARGS__)
#    endif
#else
#    define TKIT_FATAL(...)
#    define TKIT_ASSERT(...)
#    define TKIT_ASSERT_BOUNDS(...)

#    define TKIT_ASSERT_RETURNS(expression, expected, ...) expression
#    define TKIT_ASSERT_NOT_RETURNS(expression, expected, ...) expression
#endif
