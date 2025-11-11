#include "tkit/utils/logging.hpp"
#include <fmt/chrono.h>
#include <chrono>

namespace TKit
{
void Log(const std::string_view p_Message, const char *p_Level, const char *p_Color)
{
    fmt::println("[{:%Y-%m-%d %H:%M}] [{}{}{}] {}", std::chrono::system_clock::now(), p_Color, p_Level,
                 TKIT_LOG_COLOR_RESET, p_Message);
}
void Log(const std::string_view p_Message, const char *p_Level, const char *p_Color, const char *p_File,
         const i32 p_Line)
{
    fmt::println("[{:%Y-%m-%d %H:%M}] [{}{}{}] [{}:{}] {}\n", std::chrono::system_clock::now(), p_Color, p_Level,
                 TKIT_LOG_COLOR_RESET, p_File, p_Line, p_Message);
}
} // namespace TKit
