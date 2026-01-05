#include "tkit/core/pch.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit::Detail
{
void LogAndBreak(const char *p_Level, const char *p_Color, const char *p_File, const i32 p_Line)
{
    Log("[TOOLKIT] Assertion failed!", p_Level, p_Color, p_File, p_Line);
    TKIT_DEBUG_BREAK();
}
void CheckOutOfBounds(const char *p_Level, const char *p_Color, const char *p_File, const i32 p_Line,
                      const usize p_Index, const usize p_Size, const std::string_view p_Head)
{
    if (p_Size > 0)
        Log("{}Out of bounds error. Trying to access a container with an illegal index ({} >= {}). Index must be "
            "smaller than size",
            p_Level, p_Color, p_File, p_Line, p_Head, p_Index, p_Size);
    else
        Log("{}Out of bounds error. Trying to index into an empty container with an index value of {}. Container "
            "must not be indexed until it has elements",
            p_Level, p_Color, p_File, p_Line, p_Head, p_Index, p_Size);
    TKIT_DEBUG_BREAK();
}
} // namespace TKit::Detail
