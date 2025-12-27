#include "tkit/utils/logging.hpp"
#include <fmt/chrono.h>
#include <chrono>

namespace ch = std::chrono;

namespace TKit::Detail
{
void Log(const std::string_view p_Message, const char *p_Level, const char *p_Color)
{
    const auto tm = ch::round<ch::seconds>(ch::system_clock::now());
    fmt::println("[{:%Y-%m-%d %H:%M:%S}] [{}{}{}] {}", tm, p_Color, p_Level, TKIT_LOG_COLOR_RESET, p_Message);
}
void Log(const std::string_view p_Message, const char *p_Level, const char *p_Color, const char *p_File,
         const i32 p_Line)
{
    const auto tm = ch::round<ch::seconds>(ch::system_clock::now());
    fmt::println("[{:%Y-%m-%d %H:%M:%S}] [{}{}{}] [{}:{}] {}\n", tm, p_Color, p_Level, TKIT_LOG_COLOR_RESET, p_File,
                 p_Line, p_Message);
}
} // namespace TKit::Detail

namespace TKit
{
fmt::runtime_format_string<> RuntimeString(const std::string_view p_String)
{
    return fmt::runtime(p_String);
}
} // namespace TKit
