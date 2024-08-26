#include "kit/core/logging.hpp"
#ifndef KIT_NO_LOGS
#    include "core/pch.hpp"
#    include <chrono>
#    include <string>
#    include <iostream>

#    ifdef KIT_COMPILER_MSVC
#        include <intrin.h>
#    endif
#    ifdef KIT_OS_LINUX
#        include <fmt/chrono.h>
#    endif

namespace KIT
{
void debugBreak() noexcept
{
#    ifdef KIT_COMPILER_CLANG
    __builtin_debugtrap();
#    elif defined(KIT_COMPILER_GCC)
    __builtin_trap();
#    elif defined(KIT_COMPILER_MSVC)
    __debugbreak();
#    elif defined(SIGTRAP)
    raise(SIGTRAP);
#    elif defined(SIGABRT)
    raise(SIGABRT);
#    endif
}

KIT_WARNING_IGNORE_PUSH
KIT_GCC_WARNING_IGNORE("-Wunused-parameter")
KIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
void logMessage(const char *p_Level, const std::string_view p_File, const i32 p_Line, const char *p_Color,
                const bool p_Crash, const std::string_view p_Message) noexcept
{
    const std::string root = KIT_ROOT_PATH;
    const usize pos = p_File.find(root);
    const std::string_view file_rel = pos == std::string::npos ? p_File : p_File.substr(pos + 1 + root.size());

    const std::string log = KIT_FORMAT("[{:%Y-%m-%d %H:%M}] [{}{}{}] [{}:{}] {}\n", std::chrono::system_clock::now(),
                                       p_Color, p_Level, KIT_LOG_COLOR_RESET, file_rel, p_Line, p_Message);
    std::cout << log;
    if (p_Crash)
        debugBreak();
}
KIT_WARNING_IGNORE_POP

void logMessageIf(bool condition, const char *p_Level, const std::string_view p_File, const i32 p_Line,
                  const char *p_Color, const bool p_Crash, const std::string_view p_Message) noexcept
{
    if (condition)
        logMessage(p_Level, p_File, p_Line, p_Color, p_Crash, p_Message);
}
} // namespace KIT

#endif