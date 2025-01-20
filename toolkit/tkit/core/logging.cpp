#ifndef __TKIT_NO_LOGS
#    include "tkit/core/pch.hpp"
#    include "tkit/core/logging.hpp"
#    include <chrono>
#    include <string>
#    include <iostream>

#    ifdef TKIT_COMPILER_MSVC
#        include <intrin.h>
#    endif
#    ifdef TKIT_OS_LINUX
#        include <fmt/chrono.h>
#    endif

namespace TKit::Detail
{
TKIT_WARNING_IGNORE_PUSH
TKIT_GCC_WARNING_IGNORE("-Wunused-parameter")
TKIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
void LogMessage(const char *p_Level, const std::string_view p_File, const i32 p_Line, const char *p_Color,
                [[maybe_unused]] const bool p_Crash, const std::string_view p_Message) noexcept
{
    if (p_Line != Limits<i32>::max())
    {
        const std::string log =
            TKIT_FORMAT("[{:%Y-%m-%d %H:%M}] [{}{}{}] [{}:{}] {}\n", std::chrono::system_clock::now(), p_Color, p_Level,
                        TKIT_LOG_COLOR_RESET, p_File, p_Line, p_Message);
        std::cout << log;
    }
    else
    {
        const std::string log = TKIT_FORMAT("[{:%Y-%m-%d %H:%M}] [{}{}{}] {}\n", std::chrono::system_clock::now(),
                                            p_Color, p_Level, TKIT_LOG_COLOR_RESET, p_Message);
        std::cout << log;
    }
    TKIT_DEBUG_BREAK_IF(p_Crash);
}
TKIT_WARNING_IGNORE_POP
} // namespace TKit::Detail

#endif