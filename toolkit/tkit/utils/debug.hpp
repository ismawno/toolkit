#pragma once

#ifdef TKIT_ENABLE_LOGGING
#    include "tkit/utils/logging.hpp"
#endif

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
#ifdef TKIT_ENABLE_INFO_MACROS
inline thread_local bool t_DisabledInfoMacros = false;
#endif

#ifdef TKIT_ENABLE_WARNING_MACROS
inline thread_local bool t_DisabledWarningMacros = false;
#endif

#ifdef TKIT_ENABLE_ERROR_MACROS
inline thread_local bool t_DisabledErrorMacros = false;
#endif

#ifdef TKIT_ENABLE_ASSERTS
inline void LogAndBreak(const std::string_view p_Message, const char *p_Level, const char *p_Color, const char *p_File,
                        const i32 p_Line)
{
    Log(p_Message, p_Level, p_Color, p_File, p_Line);
    TKIT_DEBUG_BREAK();
}
#endif
} // namespace TKit::Detail

#ifdef TKIT_ENABLE_DEBUG_MACROS
#    define TKIT_LOG_DEBUG(...)                                                                                        \
        if (!TKit::Detail::t_DisabledInfoMacros)                                                                       \
        TKit::Log(TKit::Format(__VA_ARGS__), "DEBUG", TKIT_LOG_COLOR_DEBUG)

#    define TKIT_LOG_DEBUG_IF(p_Condition, ...)                                                                        \
        if ((p_Condition) && !TKit::Detail::t_DisabledInfoMacros)                                                      \
        TKit::Log(TKit::Format(__VA_ARGS__), "DEBUG", TKIT_LOG_COLOR_DEBUG)

#    define TKIT_LOG_DEBUG_IF_RETURNS(expression, expected, ...)                                                       \
        TKIT_LOG_DEBUG_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_DEBUG_IF_NOT_RETURNS(expression, expected, ...)                                                   \
        TKIT_LOG_DEBUG_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_DEBUG_MACROS_PUSH() TKit::Detail::t_DisabledInfoMacros = true
#    define TKIT_IGNORE_DEBUG_MACROS_POP() TKit::Detail::t_DisabledInfoMacros = false

#else
#    define TKIT_LOG_DEBUG(p_Message, ...)
#    define TKIT_LOG_DEBUG_IF(p_Condition, p_Message, ...)

#    define TKIT_LOG_DEBUG_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_DEBUG_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_DEBUG_MACROS_PUSH()
#    define TKIT_IGNORE_DEBUG_MACROS_POP()
#endif

#ifdef TKIT_ENABLE_INFO_MACROS
#    define TKIT_LOG_INFO(...)                                                                                         \
        if (!TKit::Detail::t_DisabledInfoMacros)                                                                       \
        TKit::Log(TKit::Format(__VA_ARGS__), "INFO", TKIT_LOG_COLOR_INFO)

#    define TKIT_LOG_INFO_IF(p_Condition, ...)                                                                         \
        if ((p_Condition) && !TKit::Detail::t_DisabledInfoMacros)                                                      \
        TKit::Log(TKit::Format(__VA_ARGS__), "INFO", TKIT_LOG_COLOR_INFO)

#    define TKIT_LOG_INFO_IF_RETURNS(expression, expected, ...)                                                        \
        TKIT_LOG_INFO_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_INFO_IF_NOT_RETURNS(expression, expected, ...)                                                    \
        TKIT_LOG_INFO_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_INFO_MACROS_PUSH() TKit::Detail::t_DisabledInfoMacros = true
#    define TKIT_IGNORE_INFO_MACROS_POP() TKit::Detail::t_DisabledInfoMacros = false

#else
#    define TKIT_LOG_INFO(p_Message, ...)
#    define TKIT_LOG_INFO_IF(p_Condition, p_Message, ...)

#    define TKIT_LOG_INFO_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_INFO_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_INFO_MACROS_PUSH()
#    define TKIT_IGNORE_INFO_MACROS_POP()
#endif

#ifdef TKIT_ENABLE_WARNING_MACROS
#    define TKIT_LOG_WARNING(...)                                                                                      \
        if (!TKit::Detail::t_DisabledWarningMacros)                                                                    \
        TKit::Log(TKit::Format(__VA_ARGS__), "WARNING", TKIT_LOG_COLOR_WARNING)

#    define TKIT_LOG_WARNING_IF(p_Condition, ...)                                                                      \
        if ((p_Condition) && !TKit::Detail::t_DisabledWarningMacros)                                                   \
        TKit::Log(TKit::Format(__VA_ARGS__), "WARNING", TKIT_LOG_COLOR_WARNING)

#    define TKIT_LOG_WARNING_IF_RETURNS(expression, expected, ...)                                                     \
        TKIT_LOG_WARNING_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_WARNING_IF_NOT_RETURNS(expression, expected, ...)                                                 \
        TKIT_LOG_WARNING_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_WARNING_MACROS_PUSH() TKit::Detail::t_DisabledWarningMacros = true
#    define TKIT_IGNORE_WARNING_MACROS_POP() TKit::Detail::t_DisabledWarningMacros = false

#else
#    define TKIT_LOG_WARNING(p_Message, ...)
#    define TKIT_LOG_WARNING_IF(p_Condition, p_Message, ...)

#    define TKIT_LOG_WARNING_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_WARNING_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_WARNING_MACROS_PUSH()
#    define TKIT_IGNORE_WARNING_MACROS_POP()
#endif

#ifdef TKIT_ENABLE_ERROR_MACROS
#    define TKIT_LOG_ERROR(...)                                                                                        \
        if (!TKit::Detail::t_DisabledErrorMacros)                                                                      \
        TKit::Log(TKit::Format(__VA_ARGS__), "ERROR", TKIT_LOG_COLOR_ERROR)

#    define TKIT_LOG_ERROR_IF(p_Condition, ...)                                                                        \
        if ((p_Condition) && !TKit::Detail::t_DisabledErrorMacros)                                                     \
        TKit::Log(TKit::Format(__VA_ARGS__), "ERROR", TKIT_LOG_COLOR_ERROR)

#    define TKIT_LOG_ERROR_IF_RETURNS(expression, expected, ...)                                                       \
        TKIT_LOG_ERROR_IF((expression) == (expected), __VA_ARGS__)
#    define TKIT_LOG_ERROR_IF_NOT_RETURNS(expression, expected, ...)                                                   \
        TKIT_LOG_ERROR_IF((expression) != (expected), __VA_ARGS__)

#    define TKIT_IGNORE_ERROR_MACROS_PUSH() TKit::Detail::t_DisabledErrorMacros = true
#    define TKIT_IGNORE_ERROR_MACROS_POP() TKit::Detail::t_DisabledErrorMacros = false

#else
#    define TKIT_LOG_ERROR(p_Message, ...)
#    define TKIT_LOG_ERROR_IF(p_Condition, p_Message, ...)

#    define TKIT_LOG_ERROR_IF_RETURNS(expression, expected, ...) expression
#    define TKIT_LOG_ERROR_IF_NOT_RETURNS(expression, expected, ...) expression

#    define TKIT_IGNORE_ERROR_MACROS_PUSH()
#    define TKIT_IGNORE_ERROR_MACROS_POP()
#endif

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
