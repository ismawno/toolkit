#include "tkit/utils/logging.hpp"
#include <fmt/chrono.h>
#include <chrono>

namespace ch = std::chrono;

namespace TKit::Detail
{
void Log(const std::string_view message, const char *level, const char *color)
{
    const auto tm = ch::round<ch::seconds>(ch::system_clock::now());
    fmt::println("[{:%Y-%m-%d %H:%M:%S}] [{}{}{}] {}", tm, color, level, TKIT_LOG_COLOR_RESET, message);
}
void Log(const std::string_view message, const char *level, const char *color, const char *file, const i32 line)
{
    const auto tm = ch::round<ch::seconds>(ch::system_clock::now());
    fmt::println("[{:%Y-%m-%d %H:%M:%S}] [{}{}{}] [{}:{}] {}\n", tm, color, level, TKIT_LOG_COLOR_RESET, file, line,
                 message);
}
} // namespace TKit::Detail

namespace TKit
{
fmt::runtime_format_string<> RuntimeString(const std::string_view string)
{
    return fmt::runtime(string);
}
} // namespace TKit
