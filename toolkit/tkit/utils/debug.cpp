#include "tkit/core/pch.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit::Detail
{
void LogAndBreak(const char *p_Level, const char *p_Color, const char *p_File, const i32 p_Line)
{
    Log("[TOOLKIT] Assertion failed!", p_Level, p_Color, p_File, p_Line);
    TKIT_DEBUG_BREAK();
}
} // namespace TKit::Detail
