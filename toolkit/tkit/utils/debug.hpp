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

#ifdef TKIT_ENABLE_ASSERTS
namespace TKit::Detail
{
template <typename... Args>
void LogAndBreak(const char *p_Level, const char *p_Color, const char *p_File, const i32 p_Line,
                 const fmt::format_string<Args...> p_Message, Args &&...p_Args)
{
    Log(p_Message, p_Level, p_Color, p_File, p_Line, std::forward<Args>(p_Args)...);
    TKIT_DEBUG_BREAK();
}
void LogAndBreak(const char *p_Level, const char *p_Color, const char *p_File, i32 p_Line);
void CheckOutOfBounds(const char *p_Level, const char *p_Color, const char *p_File, i32 p_Line, usize p_Index,
                      usize p_Size, std::string_view p_Head = "");
} // namespace TKit::Detail
#endif

#ifdef TKIT_ENABLE_ASSERTS
#    define TKIT_FATAL(...)                                                                                            \
        TKit::Detail::LogAndBreak("FATAL", TKIT_LOG_COLOR_FATAL, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_ASSERT(p_Condition, ...)                                                                              \
        if (!(p_Condition))                                                                                            \
        TKit::Detail::LogAndBreak("FATAL", TKIT_LOG_COLOR_FATAL, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_CHECK_OUT_OF_BOUNDS(p_Index, p_Size, ...)                                                             \
        if ((p_Index) >= (p_Size))                                                                                     \
            TKit::Detail::CheckOutOfBounds("FATAL", TKIT_LOG_COLOR_FATAL, __FILE__, __LINE__, p_Index,                 \
                                           p_Size __VA_OPT__(, ) __VA_ARGS__);

#    define TKIT_CHECK_RETURNS(p_Expression, p_Expected, ...)                                                          \
        TKIT_ASSERT((p_Expression) == (p_Expected)__VA_OPT__(, ) __VA_ARGS__)
#    define TKIT_CHECK_NOT_RETURNS(p_Expression, p_Expected, ...)                                                      \
        TKIT_ASSERT((p_Expression) != (p_Expected)__VA_OPT__(, ) __VA_ARGS__)
#else
#    define TKIT_FATAL(p_Message, ...)
#    define TKIT_ASSERT(p_Condition, p_Message, ...)

#    define TKIT_CHECK_RETURNS(p_Expression, p_Expected, ...) p_Expression
#    define TKIT_CHECK_NOT_RETURNS(p_Expression, p_Expected, ...) p_Expression
#endif
