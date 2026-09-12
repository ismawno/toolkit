#include "tkit/utils/logging.hpp"
#include "tkit/utils/limits.hpp"
#include <fmt/chrono.h>
#include <chrono>

namespace ch = std::chrono;

namespace TKit
{
static std::function<void(const LogInfo &)> s_Callback = nullptr;

const std::function<void(const LogInfo &)> &GetLogCallback()
{
    return s_Callback;
}
void SetLogCallback(const std::function<void(const LogInfo &)> &func)
{
    s_Callback = func;
}

fmt::runtime_format_string<> RuntimeFormatString(const std::string_view string)
{
    return fmt::runtime(string);
}
} // namespace TKit

namespace TKit::Detail
{
void Log(const std::string_view message, const char *level, const char *color)
{
    if (s_Callback)
        s_Callback({message, level, color, nullptr, TKIT_I32_MAX});
    const auto tm = ch::round<ch::seconds>(ch::system_clock::now());
    fmt::println("[{:%Y-%m-%d %H:%M:%S}] [{}{}{}] {}", tm, color, level, TKIT_LOG_COLOR_RESET, message);
}
void Log(const std::string_view message, const char *level, const char *color, const char *file, const i32 line)
{
    if (s_Callback)
        s_Callback({message, level, color, file, line});
    const auto tm = ch::round<ch::seconds>(ch::system_clock::now());
    fmt::println("[{:%Y-%m-%d %H:%M:%S}] [{}{}{}] [{}:{}] {}\n", tm, color, level, TKIT_LOG_COLOR_RESET, file, line,
                 message);
}
} // namespace TKit::Detail
